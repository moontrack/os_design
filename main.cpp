#include <iostream>
#include <cstring>
#include <vector>
#include <cstdio>
using namespace std;
typedef unsigned int uint;
#define cmdHelp "help"
#define cmdCreateFile "createFile"
#define cmdDeleteFile "deleteFile"
#define cmdCreateDir "createDir"
#define cmdDeleteDir "deleteDir"
#define cmdChangeDir "changeDir"
#define cmdDir "dir"
#define cmdCp "cp"
#define cmdSum "sum"
#define cmdCat "cat"
#define cmdExit "exit"

#define MAX_STORAGE_SIZE 16777216 //最大磁盘空间 16 * 1024 * 1024 = 16777216
#define INODE_SIZE 64
#define INODE_NUM 100
#define BLOCK_SIZE 1024
#define BLOCK_NUM 100
#define ADDRESS_LEN 3
#define DIRECT_BLOCK_NUM 10
#define INDIRECT_BLOCK_NUM 1
#define DIRECTORY_SIZE 20
#define FILE_NAME_LEN 20

// INODE_SIZE = 64 B
// INODE_NUM = ?
// SuperBlock_SIZE = 16 B
// BLOCK_SIZE = 1024 B
// BLOCK_NUM = ?
// INODE_SIZE * INODE_NUM + BLOCK_SIZE * BLOCK_NUM + SuperBlock_SIZE = 16MB = 16777216 B

// X * (64 + 1024) + 16 = 16777216


#pragma region Struct
// 引导块 超级块 空闲空间管理 INODE 根目录 文件和目录

// size=64
struct INODE
{
	uint ino;                                   // INODE号
	uint direct[DIRECT_BLOCK_NUM];              // 直接
	uint indirect[INDIRECT_BLOCK_NUM];          // 间接
	uint links;                                 // 链接数
	uint size;                                  // 大小
	uint createTime;                            // 时间
	int fmode;                                  // 文件类型
};
// 超级块 size = ?
struct SuperBlock
{
	uint inodeNum;
	uint finodeNum;
	uint blockNum;
	uint fblockNum;
};
// 文件夹元素
struct DirectoryElement
{
	char fileName[FILE_NAME_LEN];
	uint ino;
};
// 文件夹
struct Directory
{
	
};

SuperBlock superBlock;
bool inodeBitmap[INODE_NUM];
bool blockBitmap[BLOCK_NUM];
// INODE区: INODE_SIZE * INODE_NUM = ?
// 根目录及文件目录信息: BLOCK_SIZE * BLOCK_NUM = ?
Directory curDirectory;
#pragma endregion


#pragma region COMMAND
// 启动载入磁盘
void Start()
{
	printf("开始载入...\n");
	// function
	printf("载入完成\n");
}
// 帮助信息
void Help()
{

}
// 欢迎信息
void WelcomeMsg()
{
	// function
	Help();
}
// 启动
void Welcome()
{
	Start();
	WelcomeMsg();
}
// 创建文件 createFile fileName fileSize (KB)
void CreateFile(char* fileName, char* fileSize)
{

}
// 删除文件 deleteFile fileName
void DeleteFile(char* fileName)
{

}
// 创建文件夹 createDir dirName
void CreateDir(char* dirName)
{

}
// 删除文件夹 deleteDir dirName
void DeleteDir(char* dirName)
{

}
// 改变当前工作路径 changeDir path
void ChangeDir(char* path)
{

}
// 列出当前目录下的文件和子目录及对应信息(SIZE CREATETIME) dir
void Dir()
{

}
// 复制文件 cp file1 file2
void Cp(char* file1, char* file2)
{

}
// 显示空间使用信息 sum
void Sum()
{

}
// 打印文件内容 cat file
void Cat(char* file)
{

}
// 退出，将内容写入磁盘
void Exit()
{

}
// 解析并处理cmd 
// 退出返回0
bool parse(char* cmd)
{
	// cmd分段
	static vector<char*> vec;
	static bool head;
	head = 1;
	vec.clear();
	for (int i = 0; cmd[i]; i++)
	{
		if (cmd[i] != ' ' && head == 1)
		{
			vec.push_back(cmd + i);
			head = 0;
		}
		if (cmd[i] == ' ')
		{
			head = 1;
			cmd[i] = '\0';
		}
	}
	for (const char* item : vec)
	{
		printf("%s\n", item);
	}
	if (vec.size() == 0) return 1; // 无内容返回
	if (strcmp(vec[0], cmdHelp) == 0)
	{
		Help();
	}
	else if (strcmp(vec[0], cmdCreateFile) == 0)
	{

	}
	else if (strcmp(vec[0], cmdDeleteFile) == 0)
	{

	}
	else if (strcmp(vec[0], cmdCreateDir) == 0)
	{

	}
	else if (strcmp(vec[0], cmdDeleteDir) == 0)
	{

	}
	else if (strcmp(vec[0], cmdChangeDir) == 0)
	{

	}
	else if (strcmp(vec[0], cmdDir) == 0)
	{

	}
	else if (strcmp(vec[0], cmdCp) == 0)
	{

	}
	else if (strcmp(vec[0], cmdSum) == 0)
	{

	}
	else if (strcmp(vec[0], cmdCat) == 0)
	{

	}
	else if (strcmp(vec[0], cmdExit) == 0)
	{
		printf("结束运行，保存数据中...\n");

		printf("保存完毕\n");
		return 0;
	}
	else
	{
		printf("无法识别命令%s\n", vec[0]);
	}
	return 1;
}
#pragma endregion

int main()
{
	// test //
	printf("INODE size = %d\n", sizeof(INODE));
	printf("bool_size = %d\n", sizeof(bool));
	uint test_A = -1;
	printf("%u\n", test_A);
	// end  //

	Welcome();
	char input[100];
	while (true)
	{
		scanf("%[^\n]", input); getchar();
		if (!parse(input)) break;
	}
}