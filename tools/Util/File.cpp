#include <dirent.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <unistd.h>
#include "File.h"
#include "Logger.h"

using namespace std;

#define    _unlink   unlink
#define    _rmdir    rmdir
#define    _access   access

namespace beton {

FILE *File::create_file(const char *file, const char *mode) {
    std::string path = file;
    std::string dir;
    size_t index = 1;
    FILE *ret = nullptr;
    while (true) {
        index = path.find('/', index) + 1;
        dir = path.substr(0, index);
        if (dir.length() == 0) {
            break;
        }
        if (_access(dir.c_str(), 0) == -1) { //access函数是查看是不是存在
            if (mkdir(dir.c_str(), 0777) == -1) {  //如果不存在就用mkdir函数来创建
                WarnL << "mkdir " << dir << " failed: " << strerror(errno);
                return nullptr;
            }
        }
    }
    if (path[path.size() - 1] != '/') {
        ret = fopen(file, mode);
    }
    return ret;
}

bool File::create_path(const char *file, unsigned int mod) {
    std::string path = file;
    std::string dir;
    size_t index = 1;
    while (1) {
        index = path.find('/', index) + 1;
        dir = path.substr(0, index);
        if (dir.length() == 0) {
            break;
        }
        if (_access(dir.c_str(), 0) == -1) { //access函数是查看是不是存在
            if (mkdir(dir.c_str(), mod) == -1) {  //如果不存在就用mkdir函数来创建
                WarnL << "mkdir " << dir << " failed: " << strerror(errno);
                return false;
            }
        }
    }
    return true;
}

//判断是否为目录
bool File::is_dir(const char *path) {
    auto dir = opendir(path);
    if (!dir) {
        return false;
    }
    closedir(dir);
    return true;
}

//判断是否为常规文件
bool File::fileExist(const char *path) {
    auto fp = fopen(path, "rb");
    if (!fp) {
        return false;
    }
    fclose(fp);
    return true;
}

//判断是否是特殊目录
bool File::is_special_dir(const char *path) {
    return strcmp(path, ".") == 0 || strcmp(path, "..") == 0;
}

//生成完整的文件路径
void get_file_path(const char *path, const char *file_name, char *file_path) {
    strcpy(file_path, path);
    if (file_path[strlen(file_path) - 1] != '/') {
        strcat(file_path, "/");
    }
    strcat(file_path, file_name);
}

int File::delete_file(const char *path) {
    DIR *dir;
    dirent *dir_info;
    char file_path[PATH_MAX];
    if (is_dir(path)) {
        if ((dir = opendir(path)) == nullptr) {
            return _rmdir(path);
        }
        while ((dir_info = readdir(dir)) != nullptr) {
            if (is_special_dir(dir_info->d_name)) {
                continue;
            }
            get_file_path(path, dir_info->d_name, file_path);
            delete_file(file_path);
        }
        auto ret = _rmdir(path);
        closedir(dir);
        return ret;
    }
    return remove(path) ? _unlink(path) : 0;
}

string File::loadFile(const char *path) {
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        return "";
    }
    fseek(fp, 0, SEEK_END);
    auto len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    string str(len, '\0');
    if (size_t(len) != fread((char *)str.data(), 1, str.size(), fp)) {
        WarnL << "fread " << path << " failed: " << strerror(errno);
    }
    fclose(fp);
    return str;
}

bool File::saveFile(const string &data, const char *path) {
    FILE *fp = fopen(path, "wb");
    if (!fp) {
        return false;
    }
    fwrite(data.data(), data.size(), 1, fp);
    fclose(fp);
    return true;
}

string File::parentDir(const string &path) {
    auto parent_dir = path;
    if (parent_dir.back() == '/') {
        parent_dir.pop_back();
    }
    auto pos = parent_dir.rfind('/');
    if (pos != string::npos) {
        parent_dir = parent_dir.substr(0, pos + 1);
    }
    return parent_dir;
}

string File::absolutePath(const string &path, const string &current_path, bool can_access_parent) {
    string currentPath = current_path;
    if (!currentPath.empty()) {
        //当前目录不为空
        if (currentPath.front() == '.') {
            //如果当前目录是相对路径，那么先转换成绝对路径
            currentPath = absolutePath(current_path, exeDir(), true);
        }
    } else {
        currentPath = exeDir();
    }

    if (path.empty()) {
        //相对路径为空，那么返回当前目录
        return currentPath;
    }

    if (currentPath.back() != '/') {
        //确保当前目录最后字节为'/'
        currentPath.push_back('/');
    }
    auto rootPath = currentPath;
    auto dir_vec = split(path, "/");
    for (auto &dir : dir_vec) {
        if (dir.empty() || dir == ".") {
            //忽略空或本文件夹
            continue;
        }
        if (dir == "..") {
            //访问上级目录
            if (!can_access_parent && currentPath.size() <= rootPath.size()) {
                //不能访问根目录之外的目录, 返回根目录
                return rootPath;
            }
            currentPath = parentDir(currentPath);
            continue;
        }
        currentPath.append(dir);
        currentPath.append("/");
    }

    if (path.back() != '/' && currentPath.back() == '/') {
        //在路径是文件的情况下，防止转换成目录
        currentPath.pop_back();
    }
    return currentPath;
}

void File::scanDir(const string &path_in, const function<bool(const string &path, bool is_dir)> &cb, bool enter_subdirectory) {
    string path = path_in;
    if (path.back() == '/') {
        path.pop_back();
    }

    DIR *pDir;
    dirent *pDirent;
    if ((pDir = opendir(path.data())) == nullptr) {
        //文件夹无效
        return;
    }
    while ((pDirent = readdir(pDir)) != nullptr) {
        if (is_special_dir(pDirent->d_name)) {
            continue;
        }
        if (pDirent->d_name[0] == '.') {
            //隐藏的文件
            continue;
        }
        string strAbsolutePath = path + "/" + pDirent->d_name;
        bool isDir = is_dir(strAbsolutePath.data());
        if (!cb(strAbsolutePath, isDir)) {
            //不再继续扫描
            break;
        }

        if (isDir && enter_subdirectory) {
            //如果是文件夹并且扫描子文件夹，那么递归扫描
            scanDir(strAbsolutePath, cb, enter_subdirectory);
        }
    }
    closedir(pDir);
}

uint64_t File::fileSize(FILE *fp, bool remain_size) {
    if (!fp) {
        return 0;
    }
    auto current = ftell64(fp);
    fseek64(fp, 0L, SEEK_END); /* 定位到文件末尾 */
    auto end = ftell64(fp); /* 得到文件大小 */
    fseek64(fp, current, SEEK_SET);
    return end - (remain_size ? current : 0);
}

uint64_t File::fileSize(const char *path) {
    if (!path) {
        return 0;
    }
    auto fp = std::unique_ptr<FILE, decltype(&fclose)>(fopen(path, "rb"), fclose);
    return fileSize(fp.get());
}

uint64_t File::diskFreeSpace(const char *dir) {
    if (!dir) {
        return 0;
    }

    struct statfs sfs;
    if (statfs(dir, &sfs) != 0) {
        return 0;
    }
    return (uint64_t)(sfs.f_bfree * sfs.f_bsize);
}

uint64_t File::diskTotalSpace(const char *dir) {
    if (!dir) {
        return 0;
    }

    struct statfs sfs;
    if (statfs(dir, &sfs) != 0) {
        return 0;
    }
    return (uint64_t)(sfs.f_blocks * sfs.f_bsize);
}

}
