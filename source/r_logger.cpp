
#include "r_utils/r_logger.h"
#include "r_utils/r_string_utils.h"
#include "r_utils/r_exception.h"
#include "r_utils/r_file.h"
#include <exception>
#include <deque>
#include <algorithm>
#include <cstring>

#ifdef IS_LINUX
#include <fnmatch.h>
#include <dirent.h>
#endif

#ifdef IS_WINDOWS
#include <io.h>
#include <fcntl.h>
#endif

using namespace r_utils;
using namespace std;

static const uint32_t MAX_LOG_FILE_SIZE = ((1024*1024) * 10);

static FILE* _log_file = nullptr;
static uint32_t _approx_bytes_logged = 0;
static string _log_dir = ".";
static string _log_prefix = "log_";

static deque<string> _get_file_names(const std::string& log_dir)
{
    deque<string> output;

#ifdef IS_WINDOWS
    wstring spec = r_string_utils::convert_multi_byte_string_to_wide_string(string("*.*"));
    struct _wfinddata_t fs;
    intptr_t handle;
    if((handle=_wfindfirst(spec.c_str(), &fs)) == -1)
        R_THROW(("Unable to read directory!"));
    do
    {
        auto name = r_string_utils::convert_wide_string_to_multi_byte_string(fs.name);
        if(name != "." && name != "..")
            output.push_back(name);
    } while(0 == _wfindnext(handle, &fs));
    _findclose(handle);
#endif
#ifdef IS_LINUX
    if(DIR* d = opendir(log_dir.c_str()))
    {
        while(struct dirent* d_entry = readdir(d))
        {
            if((strncmp(".", d_entry->d_name, 1) != 0) && (strncmp("..", d_entry->d_name, 2) != 0))
                output.push_back(string(d_entry->d_name));
        }
        closedir(d);
    }
    else
        R_THROW(("Unable to opendir() requested path."));
#endif

    return output;
}

static string _next_log_name(const string& log_dir, const string& log_prefix, bool use_last = false)
{
    auto file_names = _get_file_names(log_dir);

    file_names.erase(
        remove_if(
            begin(file_names),
            end(file_names),
            [&log_prefix](const string& filename)
            {
                return !r_string_utils::starts_with(filename, log_prefix);
            }
        ),
        end(file_names)
    );

    sort(begin(file_names), end(file_names));

    auto n_file_names = file_names.size();

    if(file_names.size() >= 5)
    {
        auto remove_path = r_string_utils::format("%s%s%s",log_dir.c_str(),PATH_SLASH.c_str(),file_names.front().c_str());
        r_fs::remove_file(remove_path);
        file_names.pop_front();
    }

    if(n_file_names >= 1)
    {
        if(use_last)
            return file_names.back();

        auto newest_log_path = file_names.back();
        auto n = r_string_utils::s_to_uint32(newest_log_path.substr(log_prefix.length()));
        ++n;
        return log_prefix + r_string_utils::uint32_to_s(n) + ".txt";
    }
    else return log_prefix + "0" + ".txt";
}

static void open_log_file(const string& filename)
{
    if(_log_file)
    {
        fclose(_log_file);
        _log_file = nullptr;
    }

#ifdef IS_WINDOWS
    _log_file = _fsopen(filename.c_str(), "a+", _SH_DENYNO);
#endif
#ifdef IS_LINUX
    _log_file = fopen(filename.c_str(), "a+");
#endif

    if(_log_file == nullptr)
        R_THROW(("Unable to open logger."));
}

void r_utils::r_logger::write(LOG_LEVEL level,
                              int line,
                              const char* file,
                              const char* format,
                              ...)
{
    if(_approx_bytes_logged > MAX_LOG_FILE_SIZE)
    {
        _approx_bytes_logged = 0;
        auto v = _next_log_name(_log_dir, _log_prefix);
        auto log_path = r_string_utils::format("%s%s%s", _log_dir.c_str(), PATH_SLASH.c_str(), v.c_str());

        open_log_file(log_path);
    }

    va_list args;
    va_start(args, format);
    r_logger::write(level, line, nullptr, format, args);
    va_end(args);
}

void r_utils::r_logger::write(LOG_LEVEL level,
                              int line,
                              const char* file,
                              const char* format,
                              va_list& args)
{
    auto msg = r_string_utils::format(format, args);
    _approx_bytes_logged += (uint32_t)msg.length();
    auto lines = r_string_utils::split(msg, "\n");

    if(_log_file)
    {
        for(auto line : lines)
        {
            fprintf(_log_file, "%s\n", line.c_str());
        }
        fflush(_log_file);
    }

#ifdef IS_LINUX
    int priority = LOG_USER;

    switch(level)
    {
        case LOG_LEVEL_CRITICAL: { priority |= LOG_CRIT; break; }
        case LOG_LEVEL_ERROR: { priority |= LOG_ERR; break; }
        case LOG_LEVEL_WARNING: { priority |= LOG_WARNING; break; }
        case LOG_LEVEL_NOTICE: { priority |= LOG_NOTICE; break; }
        case LOG_LEVEL_INFO: { priority |= LOG_INFO; break; }
        case LOG_LEVEL_TRACE: { priority |= LOG_INFO; break; }
        case LOG_LEVEL_DEBUG: { priority |= LOG_INFO; break; }
        default: break;
    };

    for(auto line : lines)
    {
        syslog(priority, "%s\n", line.c_str());
    }
#endif
}

void r_utils_terminate()
{
    R_LOG_CRITICAL("r_utils terminate handler called!");
    printf("r_utils terminate handler called!\n");

    std::exception_ptr p = std::current_exception();

    if(p)
    {
        try
        {
            std::rethrow_exception(p);
        }
        catch(std::exception& ex)
        {
            R_LOG_EXCEPTION_AT(ex, __FILE__, __LINE__);
        }
        catch(...)
        {
            R_LOG_CRITICAL("unknown exception in r_utils_terminate().");
        }
    }

    fflush(stdout);

    std::abort();
}

void r_utils::r_logger::install_logger(const std::string& log_dir, const std::string& log_prefix)
{
    _log_dir = log_dir;
    _log_prefix = log_prefix;

    auto v = _next_log_name(_log_dir, _log_prefix, true);
    auto log_path = r_string_utils::format("%s%s%s", _log_dir.c_str(), PATH_SLASH.c_str(), v.c_str());

    _approx_bytes_logged = (r_fs::file_exists(log_path))?(uint32_t)r_fs::file_size(log_path):0;

    open_log_file(log_path);
}

void r_utils::r_logger::uninstall_logger()
{
    if(_log_file)
    {
        fclose(_log_file);
        _log_file = nullptr;
    }
}

void r_utils::r_logger::install_terminate()
{
    set_terminate(r_utils_terminate);
}
