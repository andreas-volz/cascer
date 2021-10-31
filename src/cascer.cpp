#include <CascLib.h>
#include <iostream>
#include <string>
#include <list>

#include "optionparser.h"

using namespace std;

// function prototypes
void CheckPath(const char* path);
int CASCListFiles(const string &storageName, const string &wildcardMask, const string &listfile);
int CASCExtractFiles(const string &storageName, list<string> &files);

// global const variables
const string appversion = "0.1";

/** option parser **/
struct Arg: public option::Arg
{
  static void printError(const char* msg1, const option::Option& opt, const char* msg2)
  {
    fprintf(stderr, "%s", msg1);
    fwrite(opt.name, opt.namelen, 1, stderr);
    fprintf(stderr, "%s", msg2);
  }

  static option::ArgStatus Unknown(const option::Option& option, bool msg)
  {
    if (msg) printError("Unknown option '", option, "'\n");

    return option::ARG_ILLEGAL;
  }

  static option::ArgStatus Required(const option::Option& option, bool msg)
  {
    if (option.arg != 0)
      return option::ARG_OK;

    if (msg) printError("Option '", option, "' requires an argument\n");
    return option::ARG_ILLEGAL;
  }

  static option::ArgStatus NonEmpty(const option::Option& option, bool msg)
  {
    if (option.arg != 0 && option.arg[0] != 0)
      return option::ARG_OK;

    if (msg) printError("Option '", option, "' requires a non-empty argument\n");
    return option::ARG_ILLEGAL;
  }

  static option::ArgStatus Numeric(const option::Option& option, bool msg)
  {
    char* endptr = 0;
    if (option.arg != 0 && strtol(option.arg, &endptr, 10)){};
    if (endptr != option.arg && *endptr == 0)
      return option::ARG_OK;

    if (msg) printError("Option '", option, "' requires a numeric argument\n");
    return option::ARG_ILLEGAL;
  }
};

enum  optionIndex { UNKNOWN, HELP, LIST, VERSION, EXTRACT, INFO, LISTFILE, MASK};
 const option::Descriptor usage[] =
 {
  {UNKNOWN, 0,"" , ""    ,option::Arg::None, "Usage: cascer [action] [option] [archive] [files]\n\n"
                                             "Action:" },
  // Action
  {HELP,    0,"h" , "help",option::Arg::None, "  --help, -h  \t\tShow this help/usage information" },
  {INFO, 	0,"i" , "info",Arg::None, "  --info, -i  \t\tShow info about archive" },
  {LIST,	0,"l" , "list",Arg::None, "  --list, -ĺ  \t\tList file(s) of archive" },
  {VERSION,	0,"V" , "version",Arg::None, "  --version, -V  \t\tShow version license information" },
  {EXTRACT,	0,"e" , "list",Arg::None, "  --extract, -e  \t\tList file(s) of archive" },
  // Options
  {UNKNOWN, 0,"" , ""    ,option::Arg::None, "\nOptions:" },
  {LISTFILE,	0,"L" , "listfile",Arg::Required, "  --listfile, -L  \t\tAdditional external listfile" },
  {MASK,	0,"m" , "mask",Arg::Required, "  --mask, -m  \t\tWildcard mask to specify files" },
  {UNKNOWN, 0,""  ,  ""   ,option::Arg::None, "\narchive \t\tDirectory which contains the game data\n"
                                              "files \t\tList of files to extract\n"},
  {0,0,0,0,0,0}
 };

 int parseOptions(int argc, const char **argv)
 {
	 string cascStorageName;
	 string cascWildcardMask;
	 string cascListfile;
	 list<string> cascFiles;

	 argc -= (argc > 0); argv += (argc > 0); // skip program name argv[0] if present
	 option::Stats  stats(usage, argc, argv);
	 option::Option options[stats.options_max], buffer[stats.buffer_max];
	 option::Parser parse(usage, argc, argv, options, buffer);

	 if(parse.error())
		 exit(0);

	 // parse Options
	 if(options[LISTFILE].count() > 0)
	 {
		 cascListfile = options[LISTFILE].arg;
	 }

	 if(options[MASK].count() > 0)
	 {
		 cascWildcardMask = options[MASK].arg;
	 }

	 // parse additional options
	 for(option::Option* opt = options[UNKNOWN]; opt; opt = opt->next())
		 std::cout << "Unknown option: " << opt->name << endl;

	 for(int i = 0; i < parse.nonOptionsCount(); ++i)
	 {
		 if(i == 0)
		 {
			 cascStorageName = parse.nonOption(i);
			 cout << "archive: " << ": " << cascStorageName << endl;
		 }
		 else
		 {
			 string cascFile = parse.nonOption(i);
			 cascFiles.push_back(cascFile);
			 cout << "file #" << i << ": " << cascFile << endl;
		 }
	 }

	 // parse internal Action
	 if(options[HELP])
	 {
		 option::printUsage(std::cout, usage);
		 exit(0);
	 }

	 if(options[VERSION].count() > 0)
	 {
		 cout << appversion << endl;
		 exit(0);
	 }

	 //CASC precondition check
	 if(cascStorageName.empty())
	 {
		 cerr << "Error: 'archive' not given!" << endl <<  endl;
		 option::printUsage(std::cout, usage);
		 exit(1);
	 }

	 if(options[INFO].count() > 0)
	 {
		 cout << "Info" << endl;
		 exit(0);
	 }

	 //parse CASC Action
	 if(options[LIST].count() > 0)
	 {
		 CASCListFiles(cascStorageName, cascWildcardMask, cascListfile);

		 exit(0);
	 }

	 if(options[EXTRACT].count() > 0)
	 {
		 CASCExtractFiles(cascStorageName, cascFiles);
		 exit(0);
	 }

	 return 0;
 }

 /**
 **  Check if path exists, if not make all directories.
 */
 void CheckPath(const char* path)
 {
 	char* cp;
 	char* s;

 #ifdef WIN32
 	cp = _strdup(path);
 #else
 	cp = strdup(path);
 #endif
 	s = strrchr(cp, '/');
 	if (s) {
 		*s = '\0';  // remove file
 		s = cp;
 		for (;;) {  // make each path element
 			s = strchr(s, '/');
 			if (s) {
 				*s = '\0';
 			}
 #if defined(_MSC_VER) || defined(WIN32)
 			_mkdir(cp);
 #else
 			mkdir(cp, 0777);
 #endif
 			if (s) {
 				*s++ = '/';
 			} else {
 				break;
 			}
 		}
 	}
 	free(cp);
 }

int CASCExtractFiles(const string &storageName, list<string> &files)
{
    HANDLE hStorage = NULL;        // Open storage handle
    HANDLE hFile  = NULL;          // Storage file handle
    DWORD dwErrCode = ERROR_SUCCESS; // Result value
    FILE *fileHandle  = NULL;

    if(dwErrCode == ERROR_SUCCESS)
    {
        if(!CascOpenStorage(storageName.c_str(), 0, &hStorage))
            cout << "Error: CascOpenStorage" << endl;
    }

    for(list<string>::iterator it = files.begin(); it != files.end(); it++)
    {
        // Access the object through iterator
        string filename = *it;

        //cout << "filename: " << filename << endl;

        // Open a file in the storage
        if(dwErrCode == ERROR_SUCCESS)
        {
            if(!CascOpenFile(hStorage, filename.c_str(), 0, 0, &hFile))
                cout << "Error: CascOpenFile" << endl;
        }

        // Read the data from the file
        if(dwErrCode == ERROR_SUCCESS)
        {
        	char  szBuffer[0x10000];
        	DWORD dwBytes = 1;

        	CheckPath(filename.c_str());
        	fileHandle = fopen(filename.c_str(), "wb");

        	while(dwBytes != 0)
        	{
        		CascReadFile(hFile, szBuffer, sizeof(szBuffer), &dwBytes);
        		if(dwBytes == 0)
        			break;

        		fwrite(szBuffer, 1, dwBytes, fileHandle);
        	}

        	if(!fileHandle)
        	{
        		fclose(fileHandle);
        	}

            if(!hFile)
            {
                CascCloseFile(hFile);
            }
        }
    }

    if(hStorage != NULL)
        CascCloseStorage(hStorage);

    return dwErrCode;
}

int CASCListFiles(const string &storageName, const string &wildcardMask, const string &listfile)
{
    HANDLE hStorage = NULL;        // Open storage handle
    HANDLE hFind = NULL; 			// find handle
    DWORD dwErrCode = ERROR_SUCCESS; // Result value
    CASC_FIND_DATA findData;

    if(dwErrCode == ERROR_SUCCESS)
    {
        if(!CascOpenStorage(storageName.c_str(), 0, &hStorage))
            cout << "Error: CascOpenStorage" << endl;
    }

    int i = 0;
    bool foundFile = true;
    hFind = CascFindFirstFile(hStorage, wildcardMask.empty() ? NULL : wildcardMask.c_str(), &findData,
    		listfile.empty() ? NULL : listfile.c_str());
    if(hFind != INVALID_HANDLE_VALUE )
	{
        while(foundFile)
        {
        	foundFile = CascFindNextFile(hFind, &findData);
        	i++;
        	if(foundFile)
        	{
        		string filename = findData.szFileName;
        		cout << filename << endl;
        	}
        }
	}

    cout << "#files found: " << i << endl;

    // Cleanup and exit
    if(hFind != NULL)
        CascFindClose(hFind);
    if(hStorage != NULL)
        CascCloseStorage(hStorage);

    return dwErrCode;
}

int main(int argc, const char **argv)
{
  parseOptions(argc, argv);

  return 0;
}
