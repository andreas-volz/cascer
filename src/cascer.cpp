#include <CascLib.h>
#include <iostream>
#include <string>
#include <list>
#include <algorithm>

#include "optionparser.h"

using namespace std;

// function prototypes
int parseOptions(int argc, const char **argv);
void CheckPath(const char* path);
int CASCListFiles(const string &storageName, const string &wildcardMask, const string &listfile, list<string> &outFiles, const string &format);
int CASCExtractFiles(const string &storageName, list<string> &files, const string &outputDir);
const string convertWinToUnixPath(const string &path);
const string convertUnixToWinPath(const string &path);
void cascPrintFile(const string &format, const string &filename);

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

enum  optionIndex { UNKNOWN, HELP, LIST, VERSION, EXTRACT, INFO, LISTFILE, MASK, OUTPUT, FORMAT};
 const option::Descriptor usage[] =
 {
  {UNKNOWN, 0,"" , ""    ,option::Arg::None, "Usage: cascer [action] [option] [archive] [files]\n\n"
                                             "Action:" },
  // Action
  {HELP,    0,"h" , "help",option::Arg::None, "  --help, -h  \t\tShow this help/usage information" },
  {INFO, 	0,"i" , "info",Arg::None, "  --info, -i  \t\tShow info about archive" },
  {LIST,	0,"l" , "list",Arg::None, "  --list, -ĺ  \t\tList file(s) of archive" },
  {VERSION,	0,"V" , "version",Arg::None, "  --version, -V  \t\tShow version license information" },
  {EXTRACT,	0,"e" , "extract",Arg::None, "  --extract, -e  \t\tExtract file(s) from archive" },
  // Options
  {UNKNOWN, 0,"" , ""    ,option::Arg::None, "\nOptions:" },
  {LISTFILE,	0,"L" , "listfile",Arg::Required, "  --listfile, -L  \t\tAdditional external listfile." },
  {MASK,	0,"m" , "mask",Arg::Required, "  --mask, -m  \t\tWildcard mask to specify files." },
  {OUTPUT,	0,"o" , "output",Arg::Required, "  --output, -o  \t\tPrefix output directory where all extracted files will be relative stored." },
  {FORMAT,	0,"f" , "format",Arg::Required, "  --format, -f  \t\tFormat of the list output. Could useful for used for extractor tools input." },
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
	 string cascOutputDir = "data";
	 string cascFormat = "%f"; // default is just the name

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

	 if(options[OUTPUT].count() > 0)
	 {
		 cascOutputDir = options[OUTPUT].arg;
	 }

	 if(options[FORMAT].count() > 0)
	 {
		 cascFormat = options[FORMAT].arg;
	 }

	 // parse additional options
	 for(option::Option* opt = options[UNKNOWN]; opt; opt = opt->next())
		 std::cout << "Unknown option: " << opt->name << endl;

	 for(int i = 0; i < parse.nonOptionsCount(); ++i)
	 {
		 if(i == 0)
		 {
			 cascStorageName = parse.nonOption(i);
			 cerr << "archive: " << ": " << cascStorageName << endl;
		 }
		 else
		 {
			 string cascFile = parse.nonOption(i);
			 cascFiles.push_back(cascFile);
			 cerr << "file #" << i << ": " << cascFile << endl;
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
		 list<string> filesResult;
		 CASCListFiles(cascStorageName, cascWildcardMask, cascListfile, filesResult, cascFormat);

		 exit(0);
	 }

	 if(options[EXTRACT].count() > 0)
	 {
		 if(cascFiles.size() == 0)
		 {
			 list<string> filesResult;
			 CASCListFiles(cascStorageName, cascWildcardMask, cascListfile, filesResult, cascFormat);
			 CASCExtractFiles(cascStorageName, filesResult, cascOutputDir);
		 }
		 else
		 {
			 CASCExtractFiles(cascStorageName, cascFiles, cascOutputDir);
			 exit(0);
		 }
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

const string convertWinToUnixPath(const string &path)
{
	string newpath = path;

	std::replace(newpath.begin(), newpath.end(), '\\', '/');

	return newpath;
}

const string convertUnixToWinPath(const string &path)
{
	string newpath = path;

	std::replace(newpath.begin(), newpath.end(), '/', '\\');

	return newpath;
}

PCASC_FILE_SPAN_INFO GetFileSpanInfo(HANDLE hFile)
{
    PCASC_FILE_SPAN_INFO pSpans = NULL;
    size_t cbLength = 0;

    // Retrieve the full file info
    CascGetFileInfo(hFile, CascFileSpanInfo, pSpans, cbLength, &cbLength);
    if(cbLength != 0)
    {
        if((pSpans = (PCASC_FILE_SPAN_INFO)(new BYTE[cbLength])) != NULL)
        {
            if(CascGetFileInfo(hFile, CascFileSpanInfo, pSpans, cbLength, NULL))
                return pSpans;

            // in case of error...
            free(pSpans);
            pSpans = NULL;
        }
    }

    return pSpans;
}

int CASCExtractFiles(const string &storageName, list<string> &files, const string &outputDir)
{
    HANDLE hStorage = NULL;        // Open storage handle
    HANDLE hFile  = NULL;          // Storage file handle
    FILE *fileHandle  = NULL;
    bool result = true;

	if(CascOpenStorage(storageName.c_str(), 0, &hStorage))
	{
		for(list<string>::iterator it = files.begin(); it != files.end(); it++)
		{
			// Access the object through iterator
			string filename = *it;

			string prefixPath = outputDir + "/" + filename;
			prefixPath = convertWinToUnixPath(prefixPath);
			//cout << "filename: " << filename << endl;
			//cout << "prefixPath: " << prefixPath << endl;

			// Open a file in the storage
			if(CascOpenFile(hStorage, filename.c_str(), 0, 0, &hFile))
			{
				// Read the data from the file
				char  szBuffer[0x10000];
				DWORD dwBytes = 1;

				// quick check if file has valid info
				// TODO: later more details to read out!
				PCASC_FILE_SPAN_INFO cascFileInfo = GetFileSpanInfo(hFile);

				if(cascFileInfo)
				{
					CheckPath(prefixPath.c_str());
					fileHandle = fopen(prefixPath.c_str(), "wb");
					cerr << "Extracting file: " << prefixPath << endl;

					while(dwBytes != 0)
					{
						CascReadFile(hFile, szBuffer, sizeof(szBuffer), &dwBytes);
						if(dwBytes == 0)
							break;

						fwrite(szBuffer, 1, dwBytes, fileHandle);
					}

					if(fileHandle != NULL)
					{
						fclose(fileHandle);
					}

					if(hFile != NULL)
					{
						CascCloseFile(hFile);
					}
				}
				else
				{
					cout << "*NOT* Extracting file (invalid info!): " << prefixPath << endl;
				}

			}
			else
			{
				cout << "Error: CascOpenFile" << endl;
				result = false;
			}
		}
	}
	else
	{
		cout << "Error: CascOpenStorage" << endl;
		result = false;
	}

    if(hStorage != NULL)
        CascCloseStorage(hStorage);

    return result;
}

int CASCListFiles(const string &storageName, const string &wildcardMask, const string &listfile, list<string> &outFiles, const string &format)
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
        	if(foundFile)
        	{
        		i++;
        		string filename = findData.szFileName;
        		outFiles.push_back(filename);
        		cascPrintFile(format, filename);
        	}
        }
	}

    cerr << "#files found: " << i << endl;

    // Cleanup and exit
    if(hFind != NULL)
        CascFindClose(hFind);
    if(hStorage != NULL)
        CascCloseStorage(hStorage);

    return dwErrCode;
}

/**
 * Use a 'date' like FORMAT string to control the output string.
 * Most useful for generating a array initializer header for other tools.
 *
 * @param format A string e.g. {X,"%f",0,"%f",1}
 */
void cascPrintFile(const string &format, const string &filename)
{
	for (auto it = format.cbegin() ; it != format.cend(); ++it)
	{
		char c = *it;

		if(c == '%')
		{
			if(it + 1 != format.cend())
			{
				it++;
			}
			char formatControl = *it;

			switch(formatControl)
			{
			case 'f':
				cout << convertWinToUnixPath(filename);
				break;
			case 'F':
				cout << convertUnixToWinPath(filename);
				break;
			default:
				//cout << "#ignore";
				break;
			}

		}
		else
		{
			cout << c;
		}
	}
	cout << endl;
}

int main(int argc, const char **argv)
{
	parseOptions(argc, argv);


	cerr << "application end" << endl;

	return 0;
}
