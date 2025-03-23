#ifndef __FILE_H__
#define __FILE_H__

#include <cstdio>
#include <cstdlib>
#include <string>
#include "Util.h"
#include <functional>
#include <limits.h>

#define fseek64 fseek
#define ftell64 ftell

namespace beton {

class File {
public:
    //创建路径
    static bool create_path(const char *file, unsigned int mod);

    //新建文件，目录文件夹自动生成
    static FILE *create_file(const char *file, const char *mode);

    //判断是否为目录
    static bool is_dir(const char *path);

    //判断是否是特殊目录（. or ..）
    static bool is_special_dir(const char *path);

    //删除目录或文件
    static int delete_file(const char *path);

    //判断文件是否存在
    static bool fileExist(const char *path);

    /**
     * 加载文件内容至string
     * @param path 加载的文件路径
     * @return 文件内容
     */
    static std::string loadFile(const char *path);

    /**
     * 保存内容至文件
     * @param data 文件内容
     * @param path 保存的文件路径
     * @return 是否保存成功
     */
    static bool saveFile(const std::string &data, const char *path);

    /**
     * 获取父文件夹
     * @param path 路径
     * @return 文件夹
     */
    static std::string parentDir(const std::string &path);

    /**
     * 替换"../"，获取绝对路径
     * @param path 相对路径，里面可能包含 "../"
     * @param current_path 当前目录
     * @param can_access_parent 能否访问父目录之外的目录
     * @return 替换"../"之后的路径
     */
    static std::string absolutePath(const std::string &path, const std::string &current_path, bool can_access_parent = false);

    /**
     * 遍历文件夹下的所有文件
     * @param path 文件夹路径
     * @param cb 回调对象 ，path为绝对路径，isDir为该路径是否为文件夹，返回true代表继续扫描，否则中断
     * @param enter_subdirectory 是否进入子目录扫描
     */
    static void scanDir(const std::string &path, const std::function<bool(const std::string &path, bool isDir)> &cb, bool enter_subdirectory = false);

    /**
     * 获取文件大小
     * @param fp 文件句柄
     * @param remain_size true:获取文件剩余未读数据大小，false:获取文件总大小
     */
    static uint64_t fileSize(FILE *fp, bool remain_size = false);

    /**
     * 获取文件大小
     * @param path 文件路径
     * @return 文件大小
     * @warning 调用者应确保文件存在
     */
    static uint64_t fileSize(const char *path);

    /**
     * 获取指定目录的磁盘剩余空间
     * @param dir 磁盘目录
     * @return 磁盘剩余空间大小Bytes
    */
    static uint64_t diskFreeSpace(const char *dir);

    /**
     * 获取指定目录的磁盘空间大小
     * @param dir 磁盘目录
     * @return 磁盘空间大小Bytes
    */
    static uint64_t diskTotalSpace(const char *dir);

private:
    File();
    ~File();
};

}
#endif  //__FILE_H__

