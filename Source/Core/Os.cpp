#include "Kaleido3D.h"
#include "Os.h"

#ifdef min
#undef min
#endif

#include <algorithm>
#include <regex>

#if K3DPLATFORM_OS_WIN
#include <process.h>
#endif

namespace Os {
File::File(const char* fileName)
  :
#if K3DPLATFORM_OS_WINDOWS
  m_hFile(NULL)
  ,
#else
  m_fd(-1)
  ,
#endif
  m_EOF(false)
  , m_CurOffset(0)
  , m_pFileName(fileName)
{
}

File::File()
  :
#if K3DPLATFORM_OS_WINDOWS
  m_hFile(NULL)
  ,
#else
  m_fd(-1)
  ,
#endif
  m_EOF(false)
  , m_CurOffset(0)
  , m_pFileName(NULL)
{
}

File::~File()
{
  Close();
}

bool
File::Open(IOFlag flag)
{
  return Open(m_pFileName, flag);
}

bool
File::Open(const char* fileName, IOFlag flag)
{
#if K3DPLATFORM_OS_WINDOWS
  wchar_t name_buf[1024];
  ::MultiByteToWideChar(CP_ACP, 0, fileName, (int)strlen(fileName) + 1, name_buf, 1024);

  DWORD shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
  int accessRights = 0;

  if (flag & IORead)
    accessRights |= GENERIC_READ;
  if (flag & IOWrite)
    accessRights |= GENERIC_WRITE;

  SECURITY_ATTRIBUTES securityAttrs = { sizeof(SECURITY_ATTRIBUTES),
                                        NULL,
                                        FALSE };
  DWORD createDisp = (flag & IOWrite) ? CREATE_ALWAYS : OPEN_EXISTING;

  m_hFile =
#if K3DPLATFORM_OS_WIN
      ::CreateFileW(name_buf, // file to open
                    accessRights,
                    shareMode,
                    &securityAttrs,
                    createDisp,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);
#else
      CreateFile2(name_buf, // file to open
          accessRights,
          shareMode,
          createDisp,
          NULL);
#endif
  if (m_hFile == INVALID_HANDLE_VALUE)
    return false;
#else
  m_fd =
    ::open(fileName, flag == IORead ? O_RDONLY : (O_WRONLY | O_CREAT), S_IRWXU);
  if (m_fd < 0) {
    int err = errno;
    if (err == EACCES) {
      perror("Permission Denied.");
    }
    return false;
  }
#endif
  return true;
}

#if K3DPLATFORM_OS_WIN

bool
File::Open(const WCHAR* fileName, IOFlag flag)
{
  DWORD shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
  int accessRights = 0;

  if (flag & IORead)
    accessRights |= GENERIC_READ;
  if (flag & IOWrite)
    accessRights |= GENERIC_WRITE;

  SECURITY_ATTRIBUTES securityAttrs = { sizeof(SECURITY_ATTRIBUTES),
                                        NULL,
                                        FALSE };
  DWORD createDisp = (flag & IOWrite) ? CREATE_ALWAYS : OPEN_EXISTING;

  m_hFile = ::CreateFileW(fileName, // file to open
                          accessRights,
                          shareMode,
                          &securityAttrs,
                          createDisp,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL);
  if (m_hFile == INVALID_HANDLE_VALUE)
    return false;
  return true;
}

#endif

int64
File::GetSize()
{
  int64 len = 0;
#if K3DPLATFORM_OS_WINDOWS
  BOOL ret = ::GetFileSizeEx(m_hFile, (PLARGE_INTEGER)&len);
  if (ret != S_OK)
  {
      return -1;
  }
#else
  struct stat st;
  if (fstat(m_fd, &st) != 0)
    return -1;
  len = st.st_size;
#endif
  return len;
}

bool
File::IsEOF()
{
  return m_EOF || m_CurOffset == GetSize();
}

size_t
File::Read(char* data, size_t len)
{
#if K3DPLATFORM_OS_WINDOWS
  if (m_hFile == INVALID_HANDLE_VALUE)
    return size_t(-1);
  DWORD bytesToRead = (DWORD)len;
  static const DWORD maxBlockSize = 32 * (1 << 20);
  int64 totalRead = 0;
  do {
    DWORD blockSize = std::min(bytesToRead, maxBlockSize);
    DWORD bytesRead;
    if (!::ReadFile(m_hFile, data + totalRead, blockSize, &bytesRead, NULL)) {
      if (totalRead == 0)
        // Error
        return size_t(-1);
      break;
    }
    if (bytesRead == 0)
      break;
    totalRead += bytesRead;
    bytesToRead -= bytesRead;
  } while (totalRead < (int64)len);

  return totalRead;
#else
  size_t _read = 0;
  _read = ::read(m_fd, data, len);
  return _read;
#endif
}

size_t
File::Write(const void* data, size_t len)
{
  size_t written = 0;
#if K3DPLATFORM_OS_WINDOWS
  WriteFile(m_hFile, data, (DWORD)len, (LPDWORD)&written, NULL);
#else
  written = ::write(m_fd, data, len);
#endif

  m_CurOffset += written;
  return written;
}

bool
File::Seek(size_t offset)
{
#if K3DPLATFORM_OS_WINDOWS
  m_CurOffset = ::SetFilePointer(m_hFile, (LONG)offset, NULL, 0);
#else
  m_CurOffset = ::lseek(m_fd, offset, SEEK_SET);
#endif
  return m_CurOffset >= 0;
}

bool
File::Skip(size_t offset)
{
#if K3DPLATFORM_OS_WINDOWS
  m_CurOffset = ::SetFilePointer(m_hFile, (LONG)offset, NULL, 1);
#else
  m_CurOffset = ::lseek(m_fd, offset, SEEK_CUR);
#endif
  return m_CurOffset >= 0;
}

void
File::Flush()
{
}

void
File::Close()
{
#if K3DPLATFORM_OS_WINDOWS
  if (m_hFile) {
    ::CloseHandle(m_hFile);
    m_hFile = NULL;
  }
#else
  if (m_fd)
    ::close(m_fd);
#endif
}

uint64
File::LastModified() const
{
#if K3DPLATFORM_OS_WINDOWS
  FILETIME FileTime = {};
  GetFileTime(m_hFile, nullptr, nullptr, &FileTime);
  return FileTime.dwLowDateTime;
#else
  return uint64();
#endif
}

File*
File::CreateIOInterface()
{
  return new File;
}
//--------------------------------------------------------------------------------------------

MemMapFile::MemMapFile()
  :
#if K3DPLATFORM_OS_WINDOWS
  m_FileHandle(NULL)
  , m_FileMappingHandle(NULL)
  ,
#else
  m_Fd(-1)
  ,
#endif
  m_szFile(0)
  , m_pData(NULL)
{
}

MemMapFile::~MemMapFile()
{
  Close();
}

int64
MemMapFile::GetSize()
{
  return m_szFile;
}

bool
MemMapFile::Open(const char* fileName, IOFlag mode)
{
  assert(mode == IORead);

#if K3DPLATFORM_OS_WINDOWS
  wchar_t name_buf[1024];
  ::MultiByteToWideChar(CP_ACP, 0, fileName, (int)strlen(fileName) + 1, name_buf, 1024);
  m_FileHandle =
#if K3DPLATFORM_OS_WIN
    ::CreateFileW(name_buf,
                  GENERIC_READ,
                  FILE_SHARE_READ,
                  NULL,
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                  NULL);
#else
      CreateFile2(name_buf, // file to open
          GENERIC_READ,
          FILE_SHARE_READ,
          OPEN_EXISTING,
          NULL);
#endif
  if (m_FileHandle == INVALID_HANDLE_VALUE)
    return false;

  auto ret = ::GetFileSizeEx(m_FileHandle, (PLARGE_INTEGER)&m_szFile);
  if (ret != S_OK && m_szFile == INVALID_FILE_SIZE)
    return false;

  m_FileMappingHandle =
    ::CreateFileMapping(m_FileHandle, NULL, PAGE_READONLY, 0, 0, NULL);
  if (m_FileMappingHandle == INVALID_HANDLE_VALUE)
    return false;

  m_pData =
    (unsigned char*)MapViewOfFile(m_FileMappingHandle, FILE_MAP_READ, 0, 0, 0);
  if (m_pData == NULL)
    return false;
#else
  m_Fd = open(fileName, O_RDONLY);
  if (m_Fd == -1)
    return false;

  struct stat st;
  if (fstat(m_Fd, &st) != 0)
    return false;

  m_szFile = st.st_size;
  if (st.st_size == 0)
    return false;

  m_pData =
    (unsigned char*)mmap(NULL, m_szFile, PROT_READ, MAP_PRIVATE, m_Fd, 0);
  if (m_pData == MAP_FAILED)
    return false;
#endif
  m_pCur = m_pData;
  return true;
}

#if K3DPLATFORM_OS_WINDOWS
bool
MemMapFile::Open(const WCHAR* fileName, IOFlag flag)
{
  m_FileHandle =
#if K3DPLATFORM_OS_WIN
      ::CreateFileW(fileName,
                  GENERIC_READ,
                  FILE_SHARE_READ,
                  NULL,
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                  NULL);
#else
      CreateFile2(fileName, // file to open
          GENERIC_READ,
          FILE_SHARE_READ,
          OPEN_EXISTING,
          NULL);
#endif
  if (m_FileHandle == INVALID_HANDLE_VALUE)
    return false;
  auto ret = ::GetFileSizeEx(m_FileHandle, (PLARGE_INTEGER)&m_szFile);
  if (ret != S_OK && m_szFile == INVALID_FILE_SIZE)
    return false;

  m_FileMappingHandle =
    ::CreateFileMapping(m_FileHandle, NULL, PAGE_READONLY, 0, 0, NULL);
  if (m_FileMappingHandle == INVALID_HANDLE_VALUE)
    return false;

  m_pData =
    (unsigned char*)MapViewOfFile(m_FileMappingHandle, FILE_MAP_READ, 0, 0, 0);
  if (m_pData == NULL)
    return false;
  return true;
}
#endif
size_t
MemMapFile::Read(char* data_ptr, size_t len)
{
  size_t bytes_to_end = m_szFile - (m_pCur - m_pData);
  if (len <= bytes_to_end) {
    memcpy(data_ptr, m_pCur, len);
    m_pCur += len;
    return len;
  }
  memcpy(data_ptr, m_pCur, bytes_to_end);
  m_pCur += bytes_to_end;
  return bytes_to_end;
}

size_t
MemMapFile::Write(const void*, size_t)
{
  assert(0 && "MemMapFile::Write(): can't write into the memory");
  return 0;
}

bool
MemMapFile::Seek(size_t offset)
{
  if (offset > m_szFile)
    return false;
  m_pCur = m_pData + offset;
  return true;
}

bool
MemMapFile::Skip(size_t offset)
{
  if (m_pCur + offset > m_pData + m_szFile)
    return false;
  m_pCur = m_pCur + offset;
  return true;
}

bool
MemMapFile::IsEOF()
{
  return (m_pCur >= m_pData + m_szFile);
}

void
MemMapFile::Flush()
{
}

void
MemMapFile::Close()
{
#if K3DPLATFORM_OS_WINDOWS
  if (m_pData) {
    UnmapViewOfFile(m_pData);
    m_pData = nullptr;
  }
  if (m_FileMappingHandle) {
    CloseHandle(m_FileMappingHandle);
    m_FileMappingHandle = NULL;
  }
#elif K3DPLATFORM_OS_UNIX
  // TODO : need fix
  munmap(m_pData, m_szFile);
  close(m_Fd);
#endif
}

MemMapFile*
MemMapFile::CreateIOInterface()
{
  return new MemMapFile;
}

struct LibraryPrivate
{
#if K3DPLATFORM_OS_WINDOWS
    HMODULE Library;
#else
    void*   Library;
#endif
    LibraryPrivate() : Library(nullptr) {}

    ~LibraryPrivate()
    {
        if (Library)
        {
#if K3DPLATFORM_OS_WINDOWS
            ::FreeLibrary(Library);
#else
            ::dlclose(Library);
#endif
            Library = nullptr;
        }
    }

    bool Load(const char* path)
    {
#if K3DPLATFORM_OS_WINDOWS
#if K3DPLATFORM_OS_WIN
        Library = LoadLibraryA(path);
#else
        WCHAR WPath[2048] = { 0 };
        MultiByteToWideChar(CP_UTF8, 0, path, strlen(path), WPath, 2048);
        Library = LoadPackagedLibrary(WPath, 0);
#endif
#else
        Library = ::dlopen(path, RTLD_LAZY);
#endif
        return Library != NULL;
    }

    void* Resolve(const char* symbol)
    {
#if K3DPLATFORM_OS_WINDOWS
        return ::GetProcAddress(Library, symbol);
#else
        return ::dlsym(Library, symbol);
#endif
    }
};



namespace Path
{
    using k3d::String;
    String Join(String const&Path0, String const&Path1)
    {
        String RetPath(64);
#if K3DPLATFORM_OS_WINDOWS
        RetPath.AppendSprintf("%s\\%s", Path0.CStr(), Path1.CStr());
#else
        RetPath.AppendSprintf("%s/%s", Path0.CStr(), Path1.CStr());
#endif
        return RetPath;
    }

    String Join(String const&Path0, String const&Path1, String const&Path2)
    {
        String RetPath(64);
#if K3DPLATFORM_OS_WINDOWS
        RetPath.AppendSprintf("%s\\%s\\%s", Path0.CStr(), Path1.CStr(), Path2.CStr());
#else
        RetPath.AppendSprintf("%s/%s/%s", Path0.CStr(), Path1.CStr(), Path2.CStr());
#endif
        return RetPath;
    }

    bool MakeDir(const char* name)
    {
#if K3DPLATFORM_OS_WINDOWS
        auto ret = CreateDirectoryA(name, nullptr);
        return TRUE == ret;
#else
        int status = mkdir(name, S_IRWXU);
        return status == 0;
#endif
    }

    bool Exists(const char* name)
    {
#if K3DPLATFORM_OS_WIN
        return TRUE == PathFileExistsA(name);
#elif K3DPLATFORM_OS_UNIX
        return ::opendir(name) != nullptr;
#endif
    }

    bool Remove(const char* lpszDir)
    {
#if K3DPLATFORM_OS_WINDOWS
        if (NULL == lpszDir || '\0' == lpszDir[0]) {
            return FALSE;
        }

        WIN32_FIND_DATAA wfd = { 0 };
        CHAR szFile[MAX_PATH] = { 0 };
        CHAR szDelDir[MAX_PATH] = { 0 };
        //MultiByteToWideChar(CP_UTF8, 0, lpszDir, strlen(lpszDir), szDelDir, MAX_PATH);
        strcpy(szDelDir, lpszDir);
        if (szDelDir[strlen(szDelDir) - 1] != '\\') {
            _snprintf(szDelDir, _countof(szDelDir) - 1, "%s\\", lpszDir);
        }
        else {
            strncpy(szDelDir, szDelDir, _countof(szDelDir) - 1);
        }
        _snprintf(szFile, _countof(szFile) - 1, "%s*.*", szDelDir);
        HANDLE hFindFile = FindFirstFileA(szFile, &wfd);
        if (INVALID_HANDLE_VALUE == hFindFile) {
            return FALSE;
        }
        do {
            if (strcmp(wfd.cFileName, ".") == 0 ||
                strcmp(wfd.cFileName, "..") == 0) {
                continue;
            }

            _snprintf(
                szFile, _countof(szFile) - 1, "%s%s", szDelDir, wfd.cFileName);
            if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                Remove(szFile);
            }
            else {
                DeleteFileA(szFile);
            }

        } while (FindNextFileA(hFindFile, &wfd));

        FindClose(hFindFile);
        RemoveDirectoryA(szDelDir);
        return true;
#else
        DIR* d = opendir(lpszDir);
        size_t path_len = strlen(lpszDir);
        int r = -1;
        if (d) {
            struct dirent* p;
            r = 0;
            while (!r && (p = readdir(d))) {
                int r2 = -1;
                char* buf;
                size_t len;

                if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, "..")) {
                    continue;
                }

                len = path_len + strlen(p->d_name) + 2;
                buf = (char*)calloc(1, len);
                if (buf) {
                    struct stat statbuf;
                    snprintf(buf, len, "%s/%s", lpszDir, p->d_name);
                    if (!stat(buf, &statbuf)) {
                        if (S_ISDIR(statbuf.st_mode)) {
                            r2 = Remove(buf);
                        }
                        else {
                            r2 = unlink(buf);
                        }
                    }
                }
                r = r2;
            }
            closedir(d);
        }
        if (!r) {
            r = rmdir(lpszDir);
        }
        return r != -1;
#endif
    }

    bool Walk(const char* srcPath, PFN_FileProcessRoutine pFn)
    {
#if K3DPLATFORM_OS_WINDOWS
        if (pFn == nullptr)
            return false;
        WIN32_FIND_DATAA ffd;
        HANDLE hFind = FindFirstFileA(srcPath, &ffd);
        if (INVALID_HANDLE_VALUE == hFind) {
            return false;
        }
        do {
            if (strcmp(ffd.cFileName, ".") != 0 && strcmp(ffd.cFileName, "..") != 0) {
                if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    pFn(ffd.cFileName, true);
                }
                else {
                    pFn(ffd.cFileName, false);
                }
            }
        } while (FindNextFileA(hFind, &ffd) != 0);
        FindClose(hFind);
        return true;
#else
        return false;
#endif
    }

    bool Copy(const char* src, const char* target)
    {
#if K3DPLATFORM_OS_WINDOWS
        return TRUE == CopyFile2((PCWSTR)src, (PCWSTR)target, nullptr);
#else
        return false;
#endif
    }
}

int
Exec(const ::k3d::kchar* cmd, ::k3d::kchar* const* argv)
{
#if K3DPLATFORM_OS_WINDOWS
#if K3DPLATFORM_OS_WIN
  return ::_wexecv(cmd, argv);
#else
  return -1;
#endif
#else
  return ::execv(cmd, argv);
#endif
}

uint64 GetTicks()
{
#if K3DPLATFORM_OS_WIN
  return ::GetTickCount64();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#endif
}

void
Sleep(uint32 ms)
{
#if K3DPLATFORM_OS_WINDOWS
  ::Sleep(ms);
#else
  ::usleep(ms * 1000);
#endif
}

#if K3DPLATFORM_OS_WINDOWS
static SYSTEM_INFO sSysInfo = {};
#endif

uint32
GetCpuCoreNum()
{
#if K3DPLATFORM_OS_WINDOWS
  if (sSysInfo.dwNumberOfProcessors == 0) {
    ::GetSystemInfo(&sSysInfo);
  }
  return sSysInfo.dwNumberOfProcessors;
#elif K3DPLATFORM_OS_UNIX
  return sysconf(_SC_NPROCESSORS_CONF);
#endif
}

struct MutexPrivate
{
#if K3DPLATFORM_OS_WINDOWS
  CRITICAL_SECTION CS;
#else
  pthread_mutex_t mMutex;
#endif
  MutexPrivate()
  {
#if K3DPLATFORM_OS_WINDOWS
    InitializeCriticalSection(&CS);
#else
    pthread_mutex_init(&mMutex, NULL);
#endif
  }

  ~MutexPrivate()
  {
#if K3DPLATFORM_OS_WINDOWS
    DeleteCriticalSection(&CS);
#else
    pthread_mutex_destroy(&mMutex);
#endif
  }

  void Lock()
  {
#if K3DPLATFORM_OS_WINDOWS
    EnterCriticalSection(&CS);
#else
    pthread_mutex_lock(&mMutex);
#endif
  }
  void UnLock()
  {
#if K3DPLATFORM_OS_WINDOWS
    LeaveCriticalSection(&CS);
#else
    pthread_mutex_unlock(&mMutex);
#endif
  }
};

Mutex::Mutex()
  : m_Impl(new MutexPrivate)
{
}

Mutex::~Mutex()
{
  delete m_Impl;
  m_Impl = nullptr;
}

void
Mutex::Lock()
{
  m_Impl->Lock();
}

void
Mutex::UnLock()
{
  m_Impl->UnLock();
}

struct ConditionVariablePrivate
{
#if K3DPLATFORM_OS_WINDOWS
  CONDITION_VARIABLE CV;
#else
  pthread_cond_t mCond;
#endif
  ConditionVariablePrivate()
  {
#if K3DPLATFORM_OS_WINDOWS
    InitializeConditionVariable(&CV);
#else
    pthread_cond_init(&mCond, NULL);
#endif
  }
  ~ConditionVariablePrivate()
  {
#if K3DPLATFORM_OS_WINDOWS
#else
    pthread_cond_destroy(&mCond);
#endif
  }

  void Wait(MutexPrivate* mutex, uint32 time)
  {
#if K3DPLATFORM_OS_WINDOWS
    ::SleepConditionVariableCS(&CV, &(mutex->CS), time);
#else
    pthread_cond_wait(&mCond, &mutex->mMutex);
#endif
  }
  void Notify()
  {
#if K3DPLATFORM_OS_WINDOWS
    ::WakeConditionVariable(&CV);
#else
    pthread_cond_signal(&mCond);
#endif
  }
  void NotifyAll()
  {
#if K3DPLATFORM_OS_WINDOWS
    ::WakeAllConditionVariable(&CV);
#else
    pthread_cond_broadcast(&mCond);
#endif
  }
};

ConditionVariable::ConditionVariable()
  : m_Impl(new ConditionVariablePrivate)
{
}

ConditionVariable::~ConditionVariable()
{
  delete m_Impl;
}

void
ConditionVariable::Wait(Mutex* mutex)
{
  if (mutex != nullptr)
    m_Impl->Wait(mutex->m_Impl, 0xffffffff);
}

void
ConditionVariable::Wait(Mutex* mutex, uint32 milliseconds)
{
  if (mutex != nullptr)
    m_Impl->Wait(mutex->m_Impl, milliseconds);
}

void
ConditionVariable::Notify()
{
  m_Impl->Notify();
}

void
ConditionVariable::NotifyAll()
{
  m_Impl->NotifyAll();
}

#define DEFAULT_THREAD_STACK_SIZE 2048

__INTERNAL_THREAD_ROUTINE_RETURN Thread::RunOnThread(void* Thr)
{
  Thread* SelfThr = static_cast<Thread*>(Thr);
  // Set ThreadName
  SetCurrentThreadName(SelfThr->GetName());
  // Run
  SelfThr->m_ThreadFunc(SelfThr->m_ThreadClosure);
  SelfThr->m_ThreadStatus = ThreadStatus::Finish;
  return 0;
}
  
  
Thread::Thread(k3d::String const& name, ThreadPriority priority)
  : m_ThreadName(name)
  , m_ThreadPriority(priority)
  , m_StackSize(DEFAULT_THREAD_STACK_SIZE)
  , m_ThreadStatus(ThreadStatus::Ready)
  , m_ThreadHandle(nullptr)
{
}

void
Thread::SleepForMilliSeconds(uint32_t millisecond)
{
  Os::Sleep(millisecond);
}

uint32_t
Thread::GetId()
{
#if K3DPLATFORM_OS_WINDOWS
  return ::GetCurrentThreadId();
#else
  return (uint64_t)pthread_self();
#endif
}

Thread::Thread()
  : Thread("", ThreadPriority::Normal)
{
}

Thread::~Thread()
{
  if (m_ThreadHandle)
  {
#if defined(K3DPLATFORM_OS_WIN)
    uint32 Tid = ::GetThreadId(m_ThreadHandle);
    //if (s_ThreadMap.find(Tid) != s_ThreadMap.end())
    //{
    //  s_ThreadMap.erase(Tid);
    //}
#endif
    if(m_ThreadClosure)
    {
      delete m_ThreadClosure;
      m_ThreadClosure = nullptr;
    }
  }
}

void
Thread::SetPriority(ThreadPriority prio)
{
  m_ThreadPriority = prio;
}

#if defined(K3DPLATFORM_OS_WIN) || defined(_WIN32)
const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push,8)  
typedef struct tagTHREADNAME_INFO
{
	DWORD dwType; // Must be 0x1000.  
	LPCSTR szName; // Pointer to name (in user addr space).  
	DWORD dwThreadID; // Thread ID (-1=caller thread).  
	DWORD dwFlags; // Reserved for future use, must be zero.  
} THREADNAME_INFO;
#pragma pack(pop)  
void SetThreadName(DWORD dwThreadID, const char* threadName) {
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = threadName;
	info.dwThreadID = dwThreadID;
	info.dwFlags = 0;
#pragma warning(push)  
#pragma warning(disable: 6320 6322)  
	__try {
		RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
	}
#pragma warning(pop)  
}

#endif

void Thread::InternalStart(ThrRoutine Routine, __internal::ThreadClosure* Closure)
{
    m_ThreadFunc = Routine;
    m_ThreadClosure = Closure;
#if K3DPLATFORM_OS_WINDOWS
    if (nullptr == m_ThreadHandle) {
      DWORD threadId;
      m_ThreadHandle = ::CreateThread(
        nullptr,
        m_StackSize,
        reinterpret_cast<LPTHREAD_START_ROUTINE>(Thread::RunOnThread),
        reinterpret_cast<LPVOID>(this),
        0,
        &threadId);
      {
        Mutex::AutoLock lock;
        DWORD tid = ::GetThreadId(m_ThreadHandle);
        m_ThreadName.AppendSprintf(" #%d", tid);
        SetThreadName(threadId, m_ThreadName.CStr());
//        s_ThreadMap[tid] = this;
      }
    }
#else
    if (0 == (u_long)m_ThreadHandle) {
      typedef void*(threadfun)(void*);
      pthread_attr_t attr;
      pthread_attr_init(&attr);
      pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
      pthread_create((pthread_t*)&m_ThreadHandle, nullptr, Thread::RunOnThread, this);
#if K3DPLATFORM_OS_ANDROID
      pthread_setname_np((pthread_t)m_ThreadHandle, m_ThreadName.CStr());
#endif
    }
#endif
}

void
Thread::Start()
{
}

void
Thread::Join()
{
  if (m_ThreadHandle != nullptr) {
#if K3DPLATFORM_OS_WINDOWS
    ::WaitForSingleObject(m_ThreadHandle, INFINITE);
#else
    void* ret;
    pthread_join((pthread_t)m_ThreadHandle, &ret);
#endif
  }
}

void
Thread::Terminate()
{
  if (m_ThreadHandle != nullptr) {
#if K3DPLATFORM_OS_WIN
    ::TerminateThread(m_ThreadHandle, 0);
#endif
  }
}

ThreadStatus
Thread::GetThreadStatus()
{
  return m_ThreadStatus;
}

k3d::String
Thread::GetName()
{
  return m_ThreadName;
}

k3d::String
Thread::GetCurrentThreadName()
{
#if K3DPLATFORM_OS_WINDOWS
  uint32 tid = (uint32)::GetCurrentThreadId();
  return "Main";
#elif K3DPLATFORM_OS_ANDROID
  char name[32];
  prctl(PR_GET_NAME, (unsigned long)name);
  return name;
#else
  pthread_t tid = pthread_self();
  return "Anonymous Thread";
#endif
}

void
Thread::SetCurrentThreadName(k3d::String const& name)
{
#if K3DPLATFORM_OS_APPLE
    pthread_setname_np(name.CStr());
#endif
}
/*
void*
Thread::Run(void* data)
{
  Thread* thr = reinterpret_cast<Thread*>(data);
  if (thr != nullptr) {
    Call call = thr->m_ThreadCallBack;
    call();
    thr->m_ThreadStatus = ThreadStatus::Finish;
#if K3DPLATFORM_OS_WIN
    ::ExitThread(0);
#else
    int ret = 0;
    pthread_exit(&ret);
#endif
  }
  return thr;
}
*/
IPv4Address::IPv4Address(const char* ip)
{
#if !K3DPLATFORM_OS_WINUWP
  ::memset(&m_Addr, 0, sizeof(m_Addr));
  m_Addr.sin_family = AF_INET;

  /*std::regex pattern1(ip_reg);
  std::regex pattern2(ip_with_port_reg);
  std::smatch match;
  const auto & ipStr = std::string(ip);
  if (std::regex_match(ipStr, match, pattern1))
  {
  #if K3DPLATFORM_OS_WIN
  m_Addr.sin_addr.S_un.S_addr = ::inet_addr(ip);
  #else
  ::inet_pton(AF_INET, ip, &m_Addr.sin_addr);
  #endif
  }
  else if (std::regex_match(ipStr, match, pattern2))
  {*/
  char ipbuff[256] = { 0 };
  std::string ipStr = ip;
  size_t pos = ipStr.find_last_of(":");
  std::string host = ipStr.substr(0, pos);
  std::string port = ipStr.substr(pos + 1, ipStr.length());
  m_Addr.sin_port = htons((u_short)(::atoi(port.c_str())));
#endif
#if K3DPLATFORM_OS_WIN
  if (host != "")
    m_Addr.sin_addr.S_un.S_addr = ::inet_addr(host.c_str());
  else
    m_Addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
#elif K3DPLATFORM_OS_UNIX
  ::inet_pton(AF_INET, ipbuff, &m_Addr.sin_addr);
#endif
  //}
  // else
  //{
  //	//error
  //}
}
void
IPv4Address::SetIpPort(uint32 port)
{
#if !K3DPLATFORM_OS_WINUWP
  m_Addr.sin_port = htons((u_short)port);
#endif
}

IPv4Address*
IPv4Address::Clone() const
{
  return new IPv4Address(*this);
}

Socket::Socket(SockType const& type)
#if K3DPLATFORM_OS_WIN
  : m_SockFd(INVALID_SOCKET)
  , m_IsBlocking(true)
#else
  : m_SockFd(-1)
#endif
  , m_SockType(type)
{
}

Socket::~Socket()
{
}

bool
Socket::IsValid()
{
#if K3DPLATFORM_OS_WIN
  return m_SockFd != INVALID_SOCKET;
#else
  return m_SockFd != -1;
#endif
}

void
Socket::SetTimeOutOpt(SoToOpt opt, uint32 milleseconds)
{
  if (!IsValid())
    return;
  struct timeval timeo;
  timeo.tv_sec = milleseconds / 1000;
  timeo.tv_usec = (milleseconds % 1000) * 1000;
  int opts[] = {
    SO_RCVTIMEO, SO_SNDTIMEO,
  };
  int ret = ::setsockopt(m_SockFd,
                         SOL_SOCKET,
                         opts[(uint32)opt],
                         (const char*)&timeo,
                         sizeof(timeo));
  if (ret == -1) {
  }
}

void
Socket::SetBlocking(bool block)
{
  if (!IsValid()) {
    return;
  }
#if K3DPLATFORM_OS_WIN
  unsigned long ul = block ? 0 : 1;
  ioctlsocket(m_SockFd, FIONBIO, &ul);
#elif K3DPLATFORM_OS_UNIX
  int flag = fcntl(m_SockFd, F_GETFL, 0);
  flag = block ? (flag & ~O_NONBLOCK) : (flag | O_NONBLOCK);
  fcntl(m_SockFd, F_SETFL, flag);
#endif
}

void
Socket::Connect(IPv4Address const& ipAddr)
{
  if (!IsValid())
      return;
#if !K3DPLATFORM_OS_WINUWP
  int rm =
    ::connect(m_SockFd, (sockaddr*)&ipAddr.m_Addr, sizeof(ipAddr.m_Addr));
  assert(rm >= 0);
#else
  //IDatagramSocket
#endif
}

void
Socket::Listen(int maxConn)
{
#if !K3DPLATFORM_OS_WINUWP
  int ret = ::listen(m_SockFd, maxConn);
  assert(ret != -1);
#endif
}

void
Socket::Bind(IPv4Address const& ipAddr)
{
#if !K3DPLATFORM_OS_WINUWP
  int ret = ::bind(m_SockFd, (sockaddr*)&ipAddr.m_Addr, sizeof(ipAddr.m_Addr));
  assert(ret != -1);
#endif
}

SocketHandle
Socket::Accept(IPv4Address& ipAddr)
{
#if K3DPLATFORM_OS_WINDOWS
  int len = sizeof(SOCKADDR);
#else
  socklen_t len = sizeof(sockaddr);
#endif

  return ::accept(m_SockFd, (sockaddr*)&ipAddr.m_Addr, &len);
}

uint64
Socket::Send(SocketHandle remote, std::string const& buffer)
{
  return ::send(remote, buffer.c_str(), (int)buffer.size(), 0);
}

uint64
Socket::Send(SocketHandle remote, const char* pData, uint32 sendLen)
{
  return ::send(remote, pData, sendLen, 0);
}

uint64
Socket::Receive(SocketHandle reomte, void* pData, uint32 recvLen)
{
  return ::recv(reomte, reinterpret_cast<char*>(pData), recvLen, 0);
  ;
}

void
Socket::Create()
{
  if (IsValid())
    return;
#if !K3DPLATFORM_OS_WINUWP
  m_SockFd = ::socket(
    AF_INET, m_SockType == SockType::TCP ? SOCK_STREAM : SOCK_DGRAM, 0);
#endif
}

void
Socket::Close()
{
  if (IsValid()) {
#if K3DPLATFORM_OS_WIN
    ::closesocket(m_SockFd);
    m_SockFd = INVALID_SOCKET;
#elif K3DPLATFORM_OS_UNIX
    close(m_SockFd);
    m_SockFd = -1;
#endif
  }
}

//========================================================
namespace RAII {
#if K3DPLATFORM_OS_WIN && !K3DPLATFORM_OS_WINUWP
struct SocketInitializer
{
  SocketInitializer()
  {
    WSADATA init;
    ::WSAStartup(MAKEWORD(2, 2), &init);
  }

  ~SocketInitializer() { ::WSACleanup(); }
};

SocketInitializer globalInitializer;
#endif
}
LibraryLoader::LibraryLoader(const char* libPath)
    : d(new LibraryPrivate)
{
    d->Load(libPath);
}
LibraryLoader::~LibraryLoader()
{
    delete d;
}
void* LibraryLoader::ResolveSymbol(const char* entryName)
{
    return d->Resolve(entryName);
}
}
