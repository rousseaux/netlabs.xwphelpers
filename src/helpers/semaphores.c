
/*
 *@@sourcefile semaphores.c:
 *      implements read-write semaphores.
 *
 *      Read-write semaphores are similar to the regular
 *      OS/2 mutex semaphores in that they are used to
 *      serialize access to a resource. See CPREF for an
 *      introduction to mutex semaphores -- do not use
 *      the things in this file if you have never used
 *      regular mutexes.
 *
 *      Regular mutexes are inefficient though if most
 *      of the access to the protected resource is
 *      read-only. It is not dangerous if several threads
 *      read from the same resource at the same time,
 *      as long as none of the threads actually modifies
 *      the resource. Still, with regular mutexes, a
 *      reading thread will be blocked out while another
 *      thread is reading, which isn't really necessary.
 *
 *      So read-write mutexes differentiate between read
 *      and write access. After creating a read-write
 *      semaphore using semCreateRWMutex,
 *
 *      --  to request read access, call semRequestRead
 *          (and semReleaseRead when done); this will
 *          let the thread in as long as no other thread
 *          has write access;
 *
 *      --  to request write access, call semRequestWrite
 *          (and semReleaseWrite when done); this will
 *          let the thread in _only_ if no other thread
 *          currently has requested either read or write
 *          access.
 *
 *      In other words, only write access is exclusive as with
 *      regular mutexes.
 *
 *      This file is new with V0.9.12 (2001-05-24) [umoeller].
 *
 *      Usage: All PM programs.
 *
 *      Function prefix:
 *
 *      --  sem*: semaphore helpers.
 *
 *@@added V0.9.12 (2001-05-24) [umoeller]
 *@@header "helpers\semaphores.h"
 */

#define OS2EMX_PLAIN_CHAR
    // this is needed for "os2emx.h"; if this is defined,
    // emx will define PSZ as _signed_ char, otherwise
    // as unsigned char

#define INCL_DOSERRORS
#define INCL_DOSSEMAPHORES

#define INCL_WINMESSAGEMGR
#include <os2.h>

#include <stdlib.h>

#include "setup.h"                      // code generation and debugging options

#include "helpers\dosh.h"
#include "helpers\semaphores.h"
#include "helpers\standards.h"
#include "helpers\tree.h"

#pragma hdrstop

/*
 *@@category: Helpers\Control program helpers\Semaphores
 *      see semaphores.c.
 */

/* ******************************************************************
 *
 *   Private declarations
 *
 ********************************************************************/

/*
 *@@ RWMUTEX:
 *      read-write mutex, as created by
 *      mtxCreateRWMutex.
 *
 *      The HRW handle is really a PRWMUTEX.
 */

typedef struct _RWMUTEX
{
    ULONG   cReaders;
                // current no. of readers on all threads,
                // including nested read requests (0 if none)
    TREE    *ReaderThreadsTree;
                // red-black tree (tree.c) containing a
                // READERTREENODE for each thread which
                // ever requested read access; items are
                // only added, but never removed (for speed)
    ULONG   cReaderThreads;
                // tree item count
                // (this is NOT the same as cReaders)

    ULONG   cWriters;
                // current no. of writers (0 if none);
                // this will only be > 1 if the same
                // thread did a nested write request,
                // since only one thread can ever have
                // write access at a time
    ULONG   tidWriter;
                // TID of current writer or 0 if none

    HEV     hevWriterDone;
                // posted after writers count goes to 0;
                // semRequestRead blocks on this

    HEV     hevReadersDone;
                // posted after readers or writers count
                // goes to 0; semRequestWrite blocks on this

} RWMUTEX, *PRWMUTEX;

/*
 *@@ READERTREENODE:
 *      tree item structure which describes
 *      a thread which requested read access.
 *
 *      These nodes are stored in RWMUTEX.ReadersTree
 *      and only ever allocated, but never removed
 *      from the tree for speed.
 *
 *      Since TREE.id holds the thread ID, this
 *      tree is sorted by thread IDs.
 */

typedef struct _READERTREENODE
{
    TREE    Tree;               // tree base struct; "id" member
                                // has the TID

    ULONG   cRequests;          // read requests from this thread;
                                // 0 after the last read request
                                // was released (since tree node
                                // won't be freed then)

} READERTREENODE, *PREADERTREENODE;

/* ******************************************************************
 *
 *   Global variables
 *
 ********************************************************************/

static HMTX         G_hmtxGlobal = NULLHANDLE;

/* ******************************************************************
 *
 *   Private helpers
 *
 ********************************************************************/

/*
 *@@ LockGlobal:
 *
 *      WARNING: As opposed to most other Lock* functions
 *      I have created, this returns an APIRET.
 *
 */

static APIRET LockGlobal(VOID)
{
    if (!G_hmtxGlobal)
        // first call:
        return (DosCreateMutexSem(NULL,
                                  &G_hmtxGlobal,
                                  0,
                                  TRUE));      // request!

    return DosRequestMutexSem(G_hmtxGlobal, SEM_INDEFINITE_WAIT);
}

/*
 *@@ UnlockGlobal:
 *
 */

static VOID UnlockGlobal(VOID)
{
    DosReleaseMutexSem(G_hmtxGlobal);
}

/* ******************************************************************
 *
 *   Public interfaces
 *
 ********************************************************************/

/*
 *@@ semCreateRWMutex:
 *      creates a read-write mutex.
 *
 *      If this returns NO_ERROR, a new RWMUTEX
 *      has been created in *ppMutex. You must
 *      use semDeleteRWMutex to free it again.
 */

APIRET semCreateRWMutex(PHRW phrw)
{
    APIRET      arc = NO_ERROR;

    if (!(arc = LockGlobal()))
    {
        PRWMUTEX    pMutex = NEW(RWMUTEX);
        if (!pMutex)
            arc = ERROR_NOT_ENOUGH_MEMORY;
        else
        {
            ZERO(pMutex);

            if (    (!(arc = DosCreateEventSem(NULL,
                                               &pMutex->hevWriterDone,
                                               0,
                                               FALSE)))
                 && (!(arc = DosCreateEventSem(NULL,
                                               &pMutex->hevReadersDone,
                                               0,
                                               FALSE)))
               )
            {
                treeInit(&pMutex->ReaderThreadsTree, NULL);
            }
        }

        if (arc)
            semDeleteRWMutex((PHRW)&pMutex);
        else
            *phrw = (HRW)pMutex;

        UnlockGlobal();
    }

    return arc;
}

/*
 *@@ semDeleteRWMutex:
 *      deletes a RW mutex previously created by
 *      semCreateRWMutex.
 *
 *      Returns:
 *
 *      --  NO_ERROR: sem was deleted, and *phrw
 *          was set to NULLHANDLE.
 *
 *      --  ERROR_SEM_BUSY: semaphore is currently
 *          requested.
 */

APIRET semDeleteRWMutex(PHRW phrw)      // in/out: rwsem handle
{
    APIRET  arc = NO_ERROR;

    if (!(arc = LockGlobal()))
    {
        PRWMUTEX pMutex;
        if (    (phrw)
             && (pMutex = (PRWMUTEX)(*phrw))
           )
        {
            if (    (pMutex->cReaders)
                 || (pMutex->cWriters)
               )
                arc = ERROR_SEM_BUSY;
            else
            {
                if (    (!(arc = DosCloseEventSem(pMutex->hevWriterDone)))
                     && (!(arc = DosCloseEventSem(pMutex->hevReadersDone)))
                   )
                {
                    LONG cItems = pMutex->cReaderThreads;
                    TREE **papNodes = treeBuildArray(pMutex->ReaderThreadsTree,
                                                     &cItems);
                    if (papNodes)
                    {
                        ULONG ul;
                        for (ul = 0; ul < cItems; ul++)
                            free(papNodes[ul]);

                        free(papNodes);
                    }

                    free(pMutex);
                    *phrw = NULLHANDLE;
                }
            }
        }
        else
            arc = ERROR_INVALID_PARAMETER;

        UnlockGlobal();
    }

    return arc;
}

/*
 *@@ semRequestRead:
 *      requests read access from the read-write mutex.
 *
 *      Returns:
 *
 *      --  NO_ERROR: caller has read access and must
 *          call semReleaseRead when done.
 *
 *      --  ERROR_INVALID_PARAMETER
 *
 *      --  ERROR_TIMEOUT
 *
 *      This function will block only if another thread
 *      currently holds write access.
 *
 *      It will not block if other threads also have
 *      write access, or it is the current thread which
 *      holds write access, or if this is a nested read
 *      request on the same thread.
 *
 *      If this function returns NO_ERROR, the calling
 *      thread is stored as a reader in the read-write
 *      mutex and will block out other threads which
 *      call semRequestWrite.
 */

APIRET semRequestRead(HRW hrw,              // in: rw-sem created by semCreateRWMutex
                      ULONG ulTimeout)      // in: timeout in ms, or SEM_INDEFINITE_WAIT
{
    APIRET  arc = NO_ERROR;
    BOOL    fLocked = FALSE;

    // protect the RW by requesting the global mutex;
    // note, we ignore ulTimeout here, since this request
    // will only ever take a very short time
    if (!(arc = LockGlobal()))
    {
        PRWMUTEX pMutex = (PRWMUTEX)hrw;
        fLocked = TRUE;

        if (!pMutex)
            arc = ERROR_INVALID_PARAMETER;
        else
        {
            // get our own thread ID
            ULONG   tidMyself = doshMyTID();

            // if there are any writers in the RW
            // besides our own thread, wait for the
            // writer to release write
            if (    (pMutex->cWriters)
                 && (pMutex->tidWriter != tidMyself)
               )
            {
                while (    (pMutex->cWriters)
                        && (!arc)
                      )
                {
                    ULONG ul;
                    DosResetEventSem(pMutex->hevWriterDone, &ul);

                    // while we're waiting on the writer to post
                    // "writers done", release the global mutex
                    UnlockGlobal();
                    fLocked = FALSE;

                    // block on "writer done"; this gets posted from
                    // semReleaseWrite after the writer has released
                    // its last write request...
                    // so after this unblocks, we must check cWriters
                    // again, in case another writer has come in between
                    if (!(arc = WinWaitEventSem(pMutex->hevWriterDone, ulTimeout)))
                        // writer done:
                        // request global mutex again
                        if (!(arc = LockGlobal()))
                            fLocked = TRUE;
                    // else: probably timeout, do not loop again
                }
            }

            if (!arc)
            {
                PREADERTREENODE pReader;

                // add readers count
                (pMutex->cReaders)++;

                // check if this thread has a reader entry already
                if (pReader = (PREADERTREENODE)treeFind(pMutex->ReaderThreadsTree,
                                                        tidMyself,  // ID to look for
                                                        treeCompareKeys))
                {
                    // yes:
                    // this is either
                    // -- a nested read request for the same thread
                    // -- or a tree item from a previous read request
                    //    which went to 0, but wasn't deleted for speed
                    //    (cRequests is then 0)
                    (pReader->cRequests)++;
                }
                else
                {
                    // no entry for this thread yet:
                    // add a new one
                    pReader = NEW(READERTREENODE);
                    if (!pReader)
                        arc = ERROR_NOT_ENOUGH_MEMORY;
                    else
                    {
                        // store the thread ID as the tree ID to
                        // sort by (so we can find by TID)
                        pReader->Tree.ulKey = tidMyself;
                        // set requests count to 1
                        pReader->cRequests = 1;

                        treeInsert(&pMutex->ReaderThreadsTree,
                                   NULL,
                                   (TREE*)pReader,
                                   treeCompareKeys);
                        (pMutex->cReaderThreads)++;
                    }
                }
            }
        }
    } // end if (!(arc = LockGlobal()))

    if (fLocked)
        UnlockGlobal();

    return arc;
}

/*
 *@@ semReleaseRead:
 *      releases read access previously requested
 *      by semRequestRead.
 *
 *      This may unblock other threads which have
 *      blocked in semRequestWrite.
 */

APIRET semReleaseRead(HRW hrw)      // in: rw-sem created by semCreateRWMutex
{
    APIRET  arc = NO_ERROR;

    // protect the RW by requesting global mutex
    if (!(arc = LockGlobal()))
    {
        PRWMUTEX pMutex = (PRWMUTEX)hrw;

        if (!pMutex)
            arc = ERROR_INVALID_PARAMETER;
        else
        {
            // get our own thread ID
            ULONG   tidMyself = doshMyTID();

            PREADERTREENODE pReader;

            // find the READERTREENODE for our TID
            if (    (pReader = (PREADERTREENODE)treeFind(pMutex->ReaderThreadsTree,
                                                         tidMyself,  // ID to look for
                                                         treeCompareKeys))
                 && (pReader->cRequests)
               )
            {
                // lower user count then (will be zero now,
                // unless read requests were nested)
                (pReader->cRequests)--;

                // note: we don't delete the tree item,
                // since it will probably be reused soon
                // (speed)

                // lower total requests count
                (pMutex->cReaders)--;

                if (pMutex->cReaders == 0)
                    // no more readers now:
                    // post "readers done" semaphore
                    DosPostEventSem(pMutex->hevReadersDone);
                            // this sets all threads blocking
                            // in semRequestWrite to "ready"
            }
            else
                // excessive releases for this thread,
                // or this wasn't requested at all:
                arc = ERROR_NOT_OWNER;
        }

        UnlockGlobal();

    } // end if (!(arc = LockGlobal()))

    return arc;
}

/*
 *@@ semQueryRead:
 *      checks if the thread currently has read
 *      access to the read-write semaphore.
 *
 *      Returns:
 *
 *      --  NO_ERROR if the same thread request
 *          read access before and thus is a reader.
 *
 *      --  ERROR_NOT_OWNER if the thread does not
 *          currently have read access.
 */

APIRET semQueryRead(HRW hrw)        // in: rw-sem created by semCreateRWMutex
{
    APIRET  arc = NO_ERROR;

    // protect the RW by requesting global mutex
    if (!(arc = LockGlobal()))
    {
        PRWMUTEX pMutex = (PRWMUTEX)hrw;

        if (!pMutex)
            arc = ERROR_INVALID_PARAMETER;
        else
        {
            // get our own thread ID
            ULONG   tidMyself = doshMyTID();

            PREADERTREENODE pReader;

            // find the READERTREENODE for our TID
            if (    (!(pReader = (PREADERTREENODE)treeFind(pMutex->ReaderThreadsTree,
                                                           tidMyself,          // ID to look for
                                                           treeCompareKeys)))
                 || (pReader->cRequests == 0)
               )
                arc = ERROR_NOT_OWNER;
            // else: pReader exists, and pReader->cRequests > 0 --> NO_ERROR
        }

        UnlockGlobal();

    } // end if (!(arc = LockGlobal()))

    return arc;
}

/*
 *@@ semRequestWrite:
 *      requests write access from the read-write mutex.
 *
 *      Returns:
 *
 *      --  NO_ERROR: caller has write access and must
 *          call semReleaseWrite when done.
 *
 *      --  ERROR_INVALID_PARAMETER
 *
 *      --  ERROR_TIMEOUT
 *
 *      This function will block if any other thread
 *      currently has read or write access.
 *
 *      It will not block if the current thread is the
 *      only thread that has requested read access
 *      before, or if this is a nested write request
 *      on the same thread.
 *
 *      If this function returns NO_ERROR, the calling
 *      thread owns the read-write mutex all alone,
 *      as if it were a regular mutex. While write
 *      access is held, other threads are blocked in
 *      semRequestRead or semRequestWrite.
 */

APIRET semRequestWrite(HRW hrw,             // in: rw-sem created by semCreateRWMutex
                       ULONG ulTimeout)     // in: timeout in ms, or SEM_INDEFINITE_WAIT
{
    APIRET  arc = NO_ERROR;
    BOOL    fLocked = FALSE;

    // protect the RW by requesting global mutex;
    // note, we ignore ulTimeout here, since this request
    // will only ever take a very short time
    if (!(arc = LockGlobal()))
    {
        PRWMUTEX pMutex = (PRWMUTEX)hrw;
        fLocked = TRUE;

        if (!pMutex)
            arc = ERROR_INVALID_PARAMETER;
        else
        {
            // get our own thread ID
            ULONG   tidMyself = doshMyTID();

            while (!arc)
            {
                // check if current TID holds read request also
                PREADERTREENODE pReader
                    = (PREADERTREENODE)treeFind(pMutex->ReaderThreadsTree,
                                                tidMyself,
                                                treeCompareKeys);
                            // != NULL if this TID has a reader already

                // let the writer in if one of the three is true:
                if (
                     // 1) no readers and no writers at all currently
                        (    (pMutex->cWriters == 0)
                          && (pMutex->cReaders == 0)
                        )
                     // or 2) there is a writer (which implies that there
                     //    are no readers), but the writer has the
                     //    same TID as the caller --> nested writer call
                     //    on the same thread
                     || (    (pMutex->cWriters)
                          && (pMutex->tidWriter == tidMyself)
                        )
                     // or 3) a reader tree item was found above, and
                     //    current thread is the only reader
                     || (    (pReader)
                          && (pReader->cRequests)
                          && (pReader->cRequests == pMutex->cReaders)
                        )
                   )
                {
                    // we're safe!
                    break;
                }
                else
                {
                    // we're NOT safe:
                    // this means that
                    // 1)   a writer other than current thread is active, or
                    // 2)   readers exist and we're not the only reader...
                    // block then until "readers done" is posted
                    ULONG ul;
                    DosResetEventSem(pMutex->hevReadersDone, &ul);

                    // while we're waiting on the last reader to post
                    // "readers done", release the global mutex
                    UnlockGlobal();
                    fLocked = FALSE;

                    // wait for all readers and writers to finish;
                    // this gets posted by
                    // -- semReleaseRead if pMutex->cReaders goes to 0
                    // -- semReleaseWrite after another writer has
                    //    released its last write request
                    if (!(arc = WinWaitEventSem(pMutex->hevReadersDone, ulTimeout)))
                        // readers done:
                        // request global mutex again
                        if (!(arc = LockGlobal()))
                            fLocked = TRUE;
                    // else: probably timeout, do not loop again
                }
            } // end while (!arc)

            if (!arc)
            {
                // OK, raise writers count
                (pMutex->cWriters)++;
                // and store our TID as the current writer
                pMutex->tidWriter = tidMyself;
            }
        }

        if (fLocked)
            UnlockGlobal();

    } // end if (!(arc = LockGlobal()))

    return arc;
}

/*
 *@@ semReleaseWrite:
 *      releases write access previously requested
 *      by semRequestWrite.
 *
 *      This may unblock other threads which have
 *      blocked in either semRequestRead or
 *      semRequestWrite.
 */

APIRET semReleaseWrite(HRW hrw)             // in: rw-sem created by semCreateRWMutex
{
    APIRET  arc = NO_ERROR;

    // protect the RW by requesting the global mutex
    if (!(arc = LockGlobal()))
    {
        PRWMUTEX pMutex = (PRWMUTEX)hrw;

        if (!pMutex)
            arc = ERROR_INVALID_PARAMETER;
        else
        {
            // get our own thread ID
            ULONG   tidMyself = doshMyTID();

            if (    (pMutex->cWriters)
                 && (pMutex->tidWriter == tidMyself)
               )
            {
                (pMutex->cWriters)--;

                if (pMutex->cWriters == 0)
                {
                    ULONG ul;
                    // last write request released:
                    // post the "writer done" semaphore
                    DosResetEventSem(pMutex->hevWriterDone, &ul);
                    DosPostEventSem(pMutex->hevWriterDone);
                            // this sets all threads blocking
                            // in semRequestRead to "ready"

                    // and post the "reader done" semaphore
                    // as well, in case another thread is
                    // waiting for write request
                    DosResetEventSem(pMutex->hevReadersDone, &ul);
                    DosPostEventSem(pMutex->hevReadersDone);
                            // this sets all threads blocking
                            // in semRequestWrite to "ready"

                    // and set tidWriter to 0
                    pMutex->tidWriter = 0;
                }
                // else: nested write request on this
                // thread (there can only ever be one
                // writer thread)
            }
            else
                // excessive releases for this thread,
                // or this wasn't requested at all:
                arc = ERROR_NOT_OWNER;
        }

        UnlockGlobal();

    } // end if (!(arc = LockGlobal()))

    return arc;
}

/*
 *@@ semQueryWrite:
 *      checks if the thread currently has write
 *      access to the read-write semaphore.
 *
 *      Returns:
 *
 *      --  NO_ERROR if the same thread request
 *          write access before and thus is a writer.
 *
 *      --  ERROR_NOT_OWNER if the thread does not
 *          currently have write access.
 */

APIRET semQueryWrite(HRW hrw)           // in: rw-sem created by semCreateRWMutex
{
    APIRET  arc = NO_ERROR;

    // protect the RW by requesting the global mutex
    if (!(arc = LockGlobal()))
    {
        PRWMUTEX pMutex = (PRWMUTEX)hrw;

        if (!pMutex)
            arc = ERROR_INVALID_PARAMETER;
        else
        {
            // get our own thread ID
            ULONG   tidMyself = doshMyTID();

            if (    (pMutex->cWriters == 0)
                 || (pMutex->tidWriter != tidMyself)
               )
                arc = ERROR_NOT_OWNER;
        }

        UnlockGlobal();

    } // end if (!(arc = LockGlobal()))

    return arc;
}


