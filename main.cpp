#include <iostream>
#include <cstring>
#include <cstdio>
using namespace std;
#define cmdCreateFile "createFile"
#define cmdDeleteFile "deleteFile"
#define cmdCreateDir "createDir"
#define cmdDeleteDir "deleteDir"
#define cmdChangeDir "changeDir"
#define cmdDir "dir"
#define cmdCp "cp"
#define cmdSum "sum"
#define cmdCat "cat"

#define MAX_STORAGE_SIZE 16777216 //最大磁盘空间 16 * 1024 * 1024 = 16777216
#define BLOCK_SIZE 1024
#define ADDRESS_LEN 3
#define DIRECT_BLOCK_NUM 10
#define INDIRECT_BLOCK_NUM 1

#pragma region Struct

#pragma endregion



#pragma region COMMAND
// 欢迎信息
void welcomeMsg()
{

}
// 创建文件 createFile fileName fileSize (KB)
void createFile(char* fileName, char* fileSize)
{

}
// 删除文件 deleteFile fileName
void deleteFile(char* fileName)
{

}
// 创建文件夹 createDir dirName
void createDir(char* dirName)
{

}
// 删除文件夹 deleteDir dirName
void deleteDir(char* dirName)
{

}
// 改变当前工作路径 changeDir path
void changeDir(char* path)
{

}
// 列出当前目录下的文件和子目录及对应信息(SIZE CREATETIME) dir
void dir()
{

}
// 复制文件 cp file1 file2
void cp(char* file1, char* file2)
{

}
// 显示空间使用信息 sum
void sum()
{

}
// 打印文件内容 cat file
void cat(char* file)
{

}
// 解析并处理cmd
void parse(char* cmd)
{

}
#pragma endregion

int main()
{

}