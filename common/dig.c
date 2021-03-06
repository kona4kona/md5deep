
// MD5DEEP - dig.c
//
// By Jesse Kornblum
//
// This is a work of the US Government. In accordance with 17 USC 105,
//  copyright protection is not available for any work of the US Government.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
//

// $Id: dig.c 362 2011-07-04 04:21:30Z jessekornblum $

#include "../md5deep/main.h"


#ifndef _WIN32
static void remove_double_slash(TCHAR *fn)
{
  size_t tsize = sizeof(TCHAR);
  TCHAR DOUBLE_DIR[4], *tmp = fn, *new;
  _sntprintf(DOUBLE_DIR,3,_TEXT("%c%c"),DIR_SEPARATOR,DIR_SEPARATOR);

  new = _tcsstr(tmp,DOUBLE_DIR);
  while (NULL != new)
  {
#ifdef _WIN32
    // On Windows, we have to allow the first two characters to be slashes
    // to account for UNC paths. e.g. \\SERVER\dir\path  
    if (tmp == fn)
    {  
      ++tmp;
    } 
    else
    {
#endif  // ifdef _WIN32
    
      _tmemmove(new,new+tsize,_tcslen(new));

#ifdef _WIN32
    }
#endif  // ifdef _WIN32

    new = _tcsstr(tmp,DOUBLE_DIR);

  }
}


static void remove_single_dirs(TCHAR *fn)
{
  unsigned int pos, chars_found = 0;
  size_t sz = _tcslen(fn), tsize = sizeof(TCHAR);

  for (pos = 0 ; pos < sz ; pos++)
  {
    // Catch strings that end with /. (e.g. /foo/.)  
    if (pos > 0 && 
	fn[pos-1] == _TEXT(DIR_SEPARATOR) && 
	fn[pos]   == _TEXT('.') &&
	fn[pos+1] == 0)
      fn[pos] = 0;
    
    if (fn[pos] == _TEXT('.') && fn[pos+1] == _TEXT(DIR_SEPARATOR))
    {
      if (chars_found && fn[pos-1] == _TEXT(DIR_SEPARATOR))
      {
	_tmemmove(fn+(pos*tsize),(fn+((pos+2)*tsize)),(sz-pos) * tsize);
	
	// In case we have ././ we shift back one! 
	--pos;

      }
    }
    else 
      ++chars_found;
  }
}

// Removes all "../" references from the absolute path fn 
void remove_double_dirs(TCHAR *fn)
{
  size_t pos, next_dir, sz = _tcslen(fn), tsize = sizeof(TCHAR);

  for (pos = 0; pos < _tcslen(fn) ; pos++)
  {
    if (fn[pos] == _TEXT('.') && fn[pos+1] == _TEXT('.'))
    {
      if (pos > 0)
      {

	// We have to keep this next if statement and the one above separate.
	// If not, we can't tell later on if the pos <= 0 or
	// that the previous character was a DIR_SEPARATOR.
	// This matters when we're looking at ..foo/ as an input */
	
	if (fn[pos-1] == _TEXT(DIR_SEPARATOR))
	{
	  
	  next_dir = pos + 2;
	  
	  // Back up to just before the previous DIR_SEPARATOR
	  // unless we're already at the start of the string */
	  if (pos > 1)
	    pos -= 2;
	  else
	    pos = 0;
	  
	  while (fn[pos] != _TEXT(DIR_SEPARATOR) && pos > 0)
	    --pos;
	  
	  switch(fn[next_dir])
	  {
	  case DIR_SEPARATOR:
	    _tmemmove(fn+pos,fn+next_dir,((sz - next_dir) + 1) * tsize);
	    break;
	    
	  case 0:
	    // If we have /.. ending the filename 
	    fn[pos+1] = 0;
	    break;
	    
	    // If we have ..foo, we should do nothing, but skip 
	    // over these double dots 
	  default:
	    pos = next_dir;
	  }
	}
      }
      
      // If we have two dots starting off the string, we should prepend
      // a DIR_SEPARATOR and ignore the two dots. That is:
      // from the root directory the path ../foo is really just /foo 
    
      else 
      {
	fn[pos] = _TEXT(DIR_SEPARATOR);
	_tmemmove(fn+pos+1,fn+pos+3,sz-(pos+3));


      }
    }
  }
}
#endif  // ifndef _WIN32


// On Win32 systems directories are handled... differently
// Attempting to process d: causes an error, but d:\ does not.
// Conversely, if you have a directory "foo",
// attempting to process d:\foo\ causes an error, but d:\foo does not.
// The following turns d: into d:\ and d:\foo\ into d:\foo 

#ifdef _WIN32
static void clean_name_win32(TCHAR *fn)
{
  size_t length = _tcslen(fn);

  if (length < 2)
    return;

  if (length == 2 && fn[1] == _TEXT(':'))
  {
    fn[length+1] = 0;
    fn[length]   = _TEXT(DIR_SEPARATOR);
    return;
  }

  if (fn[length-1] == _TEXT(DIR_SEPARATOR) && length != 3)
  {
    fn[length - 1] = 0;
  }
}

static int is_win32_device_file(TCHAR *fn)
{
  // Specifications for device files came from
  // http://msdn.microsoft.com/en-us/library/aa363858(VS.85).aspx
  // Physical devices (like hard drives) are 
  //   \\.\PhysicalDriveX where X is a digit from 0 to 9
  // Tape devices are \\.\tapeX where X is a digit from 0 to 9
  // Logical volumes are \\.\X: where X is a letter */

  if (!_tcsnicmp(fn, _TEXT("\\\\.\\physicaldrive"),17) &&
      (_tcslen(fn) == 18) && 
      iswdigit(fn[17]))
    return TRUE;

  if (!_tcsnicmp(fn, _TEXT("\\\\.\\tape"),8) &&
      (_tcslen(fn) == 9) && 
      iswdigit(fn[8]))
    return TRUE;
 
  if ((!_tcsnicmp(fn,_TEXT("\\\\.\\"),4)) &&
      (_tcslen(fn) == 6) &&
      (iswalpha(fn[4])) &&
      (fn[5] == _TEXT(':')))
    return TRUE;

  return FALSE;
}

#endif  // ifdef _WIN32 


static void clean_name(state *s, TCHAR *fn)
{
  if (NULL == s)
    return;

#ifdef _WIN32
  clean_name_win32(fn);
#else

  // We don't need to call these functions when running in Windows
  // as we've already called real_path() on them in main.c. These
  // functions are necessary in *nix so that we can clean up the 
  // path names without removing the names of symbolic links. They
  // are also called when the user has specified an absolute path
  // but has included extra double dots or such.
  //
  // TODO: See if Windows Vista's symbolic links create problems 

  if (!(s->mode & mode_relative))
  {
    remove_double_slash(fn);
    remove_single_dirs(fn);
    remove_double_dirs(fn);
  }
#endif
}


//  Debugging function
/*
#ifdef _WIN32
static void print_last_error(char * function_name)
{
  // Copied from http://msdn.microsoft.com/en-us/library/ms680582(VS.85).aspx
  // Retrieve the system error message for the last-error code

  LPTSTR pszMessage;
  DWORD dwLastError = GetLastError(); 

  FormatMessage((FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		 FORMAT_MESSAGE_FROM_SYSTEM |
		 FORMAT_MESSAGE_IGNORE_INSERTS),
		NULL,
		dwLastError,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &pszMessage,
		0, NULL );
  
  // Display the error message and exit the process
  fprintf(stdout,"%s failed with error %ld: ", function_name, dwLastError);
  display_filename(stdout,pszMessage);
  
  LocalFree(pszMessage);
}
#endif
*/

// An NTFS Junction Point is like a hard link on *nix but only works
// on the same filesystem and only for directories. Unfortunately they
// can also create infinite loops for programs that recurse filesystems.
// See http://blogs.msdn.com/oldnewthing/archive/2004/12/27/332704.aspx 
// for an example of such an infinite loop.
//
// This function detects junction points and returns TRUE if the
// given filename is a junction point. Otherwise it returns FALSE.
static int is_junction_point(state *s, TCHAR *fn)
{
  int status = FALSE;

  if (NULL == s || NULL == fn)
    return FALSE;


#ifdef _WIN32
  WIN32_FIND_DATA FindFileData;
  HANDLE hFind;

  hFind = FindFirstFile(fn, &FindFileData);
  if (INVALID_HANDLE_VALUE != hFind)
  {
    if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
    {
      // We're going to skip this reparse point no matter what,
      // but we may want to display a message just in case.
      // TODO: Maybe have the option to follow symbolic links?
      status = TRUE;

      if (IO_REPARSE_TAG_MOUNT_POINT == FindFileData.dwReserved0)
      {
	print_error_unicode(s,fn,"Junction point, skipping");
      }
      else if (IO_REPARSE_TAG_SYMLINK == FindFileData.dwReserved0)
      {
	print_error_unicode(s,fn,"Symbolic link, skipping");
      }	
      else 
      {
	print_error_unicode(s,fn,"Unknown reparse point 0x%"PRIx32", skipping",
			    FindFileData.dwReserved0);
      }
    }

    // We don't error check this call as there's nothing to do differently
    // if it fails.
    FindClose(hFind);
  }
#endif

  return status;
}

// This is experimental code for reparse point process
// We don't use it yet, but I don't want to delete it
// until I know what I'm doing. (jk 1 Mar 2009)
/*
  #include <ddk/ntifs.h>
  
    if (status)
    {
      HANDLE hFile = CreateFile(fn,
				0,   // desired access
				FILE_SHARE_READ,
				NULL,  
				OPEN_EXISTING,
				(FILE_FLAG_OPEN_REPARSE_POINT | 
				 FILE_FLAG_BACKUP_SEMANTICS),
				NULL);

      if (INVALID_HANDLE_VALUE == hFile)
      {
	print_last_error(L"CreateFile");
      }
      else
      {
	print_status("Opened ok!");

	REPARSE_DATA_BUFFER buf;
	uint8_t bytesReturned;
	LPOVERLAPPED ov;

	int rc = DeviceIoControl(hFile,
				 FSCTL_GET_REPARSE_POINT,
				 NULL,
				 0,
				 &buf,
				 MAXIMUM_REPARSE_DATA_BUFFER_SIZE,
				 &bytesReturned,
				 ov);
	if (!rc)
	{
	  print_last_error(L"DeviceIoControl");
	}

	CloseHandle(hFile);
      }
    }

    FindClose(hFind);
    */




// Returns TRUE if the directory is '.' or '..', otherwise FALSE
static int is_special_dir(TCHAR *d)
{
  return ((!_tcsncmp(d,_TEXT("."),1) && (_tcslen(d) == 1)) ||
          (!_tcsncmp(d,_TEXT(".."),2) && (_tcslen(d) == 2)));
}



static int process_dir(state *s, TCHAR *fn)
{
  int return_value = STATUS_OK;
  TCHAR *new_file;
  _TDIR *current_dir;
  struct _tdirent *entry;

  //  print_status (_TEXT("Called process_dir(%s)"), fn);

  if (have_processed_dir(fn))
  {
    print_error_unicode(s,fn,"symlink creates cycle");
    return STATUS_OK;
  }

  if (!processing_dir(fn))
    internal_error("%s: Cycle checking failed to register directory.", fn);
  
  if ((current_dir = _topendir(fn)) == NULL) 
  {
    print_error_unicode(s,fn,"%s", strerror(errno));
    return STATUS_OK;
  }    

  new_file = (TCHAR *)malloc(sizeof(TCHAR) * PATH_MAX);
  if (NULL == new_file)
    internal_error("%s: Out of memory", __progname);

  while ((entry = _treaddir(current_dir)) != NULL) 
  {
    if (is_special_dir(entry->d_name))
      continue;
    
    _sntprintf(new_file,PATH_MAX,_TEXT("%s%c%s"),
	       fn,DIR_SEPARATOR,entry->d_name);

    if (is_junction_point(s,new_file))
      continue;

    return_value = process_normal(s,new_file);

  }
  free(new_file);
  _tclosedir(current_dir);
  
  if (!done_processing_dir(fn))
    internal_error("%s: Cycle checking failed to unregister directory.", fn);

  return return_value;
}


static int file_type_helper(_tstat_t sb)
{
  if (S_ISREG(sb.st_mode))
    return stat_regular;
  
  if (S_ISDIR(sb.st_mode))
    return stat_directory;
  
  if (S_ISBLK(sb.st_mode))
    return stat_block;
  
  if (S_ISCHR(sb.st_mode))
    return stat_character;
  
  if (S_ISFIFO(sb.st_mode))
    return stat_pipe;

  // These file types do not exist in Win32 
#ifndef _WIN32
  
  if (S_ISSOCK(sb.st_mode))
    return stat_socket;
  
  if (S_ISLNK(sb.st_mode))
    return stat_symlink;  
#endif   // ifndef _WIN32 

  // Used to detect Solaris doors 
#ifdef S_IFDOOR
#ifdef S_ISDOOR
  if (S_ISDOOR(sb.st_mode))
    return stat_door;
#endif
#endif

  return stat_unknown;
}


// Use a stat function to look up while kind of file this is
// and, if possible, it's size.
static int file_type(state *s, TCHAR *fn)
{
  _tstat_t sb;

  if (_lstat(fn,&sb))
  {
    print_error_unicode(s,fn,"%s", strerror(errno));
    return stat_unknown;
  }

  s->stat_bytes = sb.st_size;

  // On Win32 this should be the creation time, but on all other systems
  // it will be the change time.
  s->timestamp = sb.st_ctime; 

  return file_type_helper(sb);
}


static int should_hash_symlink(state *s, TCHAR *fn, int *link_type);

#define RETURN_IF_MODE(A) \
if (s->mode & A) \
  return TRUE; \
break;


// Type should be the result of calling lstat on the file. We want to
// know what this file is, not what it points to
static int should_hash_expert(state *s, TCHAR *fn, int type)
{
  int link_type;

  switch(type)
  {

  case stat_directory:
    if (s->mode & mode_recursive)
      process_dir(s,fn);
    else
    {
      print_error_unicode(s,fn,"Is a directory");
    }
    return FALSE;

    // We can't just return s->mode & mode_X because mode_X is
    // a 64-bit value. When that value gets converted back to int,
    // the high part of it is lost. 

  case stat_regular:   RETURN_IF_MODE(mode_regular);

  case stat_block:     RETURN_IF_MODE(mode_block);
    
  case stat_character: RETURN_IF_MODE(mode_character);

  case stat_pipe:      RETURN_IF_MODE(mode_pipe);

  case stat_socket:    RETURN_IF_MODE(mode_socket);
    
  case stat_door:      RETURN_IF_MODE(mode_door);

  case stat_symlink: 

    //  Although it might appear that we need nothing more than
    //     return (s->mode & mode_symlink);
    // that doesn't work. That logic gets into trouble when we're
    // running in recursive mode on a symlink to a directory.
    // The program attempts to open the directory entry itself
    // and gets into an infinite loop.

    if (!(s->mode & mode_symlink))
      return FALSE;

    if (should_hash_symlink(s,fn,&link_type))
      return should_hash_expert(s,fn,link_type);
    else
      return FALSE;
  }
  
  return FALSE;
}


static int should_hash_symlink(state *s, TCHAR *fn, int *link_type)
{
  int type;
  _tstat_t sb;

  // We must look at what this symlink points to before we process it.
  // The normal file_type function uses lstat to examine the file,
  // we use stat to examine what this symlink points to. 
  if (_sstat(fn,&sb))
  {
    print_error_unicode(s,fn,"%s",strerror(errno));
    return FALSE;
  }

  type = file_type_helper(sb);

  if (type == stat_directory)
  {
    if (s->mode & mode_recursive)
      process_dir(s,fn);
    else
    {
      print_error_unicode(s,fn,"Is a directory");
    }
    return FALSE;
  }    

  if (link_type != NULL)
    *link_type = type;
  return TRUE;    
}
    




static int should_hash(state *s, TCHAR *fn)
{
  int type;

  // We must reset the number of bytes in each file processed
  // so that we can tell if fstat reads the number successfully
  s->stat_bytes = UNKNOWN_FILE_SIZE;
  s->timestamp   = 0;

  type = file_type(s,fn);
  
  if (s->mode & mode_expert)
    return (should_hash_expert(s,fn,type));

  if (type == stat_directory)
  {
    if (s->mode & mode_recursive)
      process_dir(s,fn);
    else 
    {
      print_error_unicode(s,fn,"Is a directory");
    }
    return FALSE;
  }

#ifndef _WIN32
  if (type == stat_symlink)
    return should_hash_symlink(s,fn,NULL);
#endif

  if (type == stat_unknown)
    return FALSE;

  // By default we hash anything we can't identify as a "bad thing"
  return TRUE;
}


int process_normal(state *s, TCHAR *fn)
{
  clean_name(s,fn);

  if (should_hash(s,fn))
    return (hash_file(s,fn));
  
  // RBF - Standardize return values for this function and audit functions
  // This function returns FALSE. hash_file, called above, returns STATUS_OK
  // process_win32 also returns STATUS_OK. 
  // display_audit_results, used by hashdeep, returns EXIT_SUCCESS/FAILURE.
  // Pick one and stay with it!
  return FALSE;
}


#ifdef _WIN32
int process_win32(state *s, TCHAR *fn)
{
  int rc, status = STATUS_OK;
  TCHAR *asterisk, *question, *tmp, *dirname, *new_fn;
  WIN32_FIND_DATA FindFileData;
  HANDLE hFind;

  //  print_status("Called process_win32(%S)", fn);

  if (is_win32_device_file(fn))
    return (hash_file(s,fn));

  // Filenames without wildcards can be processed by the
  // normal recursion code.
  asterisk = _tcschr(fn,L'*');
  question = _tcschr(fn,L'?');
  if (NULL == asterisk && NULL == question)
    return (process_normal(s,fn));
  
  hFind = FindFirstFile(fn, &FindFileData);
  if (INVALID_HANDLE_VALUE == hFind)
  {
    print_error_unicode(s,fn,"No such file or directory");
    return STATUS_OK;
  }
  
#define FATAL_ERROR_UNK(A) if (NULL == A) fatal_error(s,"%s: %s", __progname, strerror(errno));
#define FATAL_ERROR_MEM(A) if (NULL == A) fatal_error(s,"%s: Out of memory", __progname);
  
  MD5DEEP_ALLOC(TCHAR,new_fn,PATH_MAX);
  
  dirname = _tcsdup(fn);
  FATAL_ERROR_MEM(dirname);
  
  my_dirname(dirname);
  
  rc = 1;
  while (0 != rc)
  {
    if (!(is_special_dir(FindFileData.cFileName)))
    {
      // The filename we've found doesn't include any path information.
      // We have to add it back in manually. Thankfully Windows doesn't
      // allow wildcards in the early part of the path. For example,
      // we will never see:  c:\bin\*\tools 
      //
      // Because the wildcard is always in the last part of the input
      // (e.g. c:\bin\*.exe) we can use the original dirname, combined
      // with the filename we've found, to make the new filename. 
      
      if (s->mode & mode_relative)
      {
	_sntprintf(new_fn,PATH_MAX,
		   _TEXT("%s%s"), dirname, FindFileData.cFileName);
      }
      else
      {	  
	MD5DEEP_ALLOC(TCHAR,tmp,PATH_MAX);
	_sntprintf(tmp,PATH_MAX,_TEXT("%s%s"),dirname,FindFileData.cFileName);
	_wfullpath(new_fn,tmp,PATH_MAX);
	free(tmp);
      }
      
      if (!(is_junction_point(s,new_fn)))
	process_normal(s,new_fn); 
    }
    
    rc = FindNextFile(hFind, &FindFileData);
  }
  
  // rc now equals zero, we can't find the next file

  if (ERROR_NO_MORE_FILES != GetLastError())
  {
    // The Windows API for getting an intelligible error message
    // is beserk. Rather than play their silly games, we 
    // acknowledge that an unknown error occured and hope we
    // can continue.
    print_error_unicode(s,fn,"Unknown error while expanding wildcard");
    free(dirname);
    free(new_fn);
    return STATUS_OK;
  }
  
  rc = FindClose(hFind);
  if (0 == rc)
  {
    print_error_unicode(s,
			fn,
			"Unknown error while cleaning up wildcard expansion");
  }

  free(dirname);
  free(new_fn);

  return status;
}
#endif


