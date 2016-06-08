#include "utils.h"
#include <dirent.h>
#include <stdexcept>

using namespace std;

static bool has_suffix(const std::string &str, const std::string &suffix)
{
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

vector<string> get_ppm_file_paths( const string& path )
{
    vector<string> names;
    DIR* d = opendir( path.c_str() );

    if( !d )
        throw runtime_error(string("Unable to open path: ") + path);

    struct dirent* entry = NULL;
    while( (entry=readdir(d)) != NULL )
    {
        string name = entry->d_name;
        if( has_suffix( name, ".ppm" ) )
            names.push_back( path + "/" + name );
    }
    closedir( d );

    return names;
}
