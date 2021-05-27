#include <iostream>
#include <cstring>
#include <vector>
#include <cstdio>
using namespace std;
typedef unsigned int uint;
typedef unsigned short ushort;
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

#pragma region PreCal
// TOTAL = 16 MB 
// BLOCK_SIZE = 1 KB
// BLOCK_NUM = 16 * 1024 = (1<<14) = 16384
// 所以24b地址长度中只有14b有效，为方便起见，使用unsigned short模拟24b地址长度

// X = BLOCK_NUM = INODE_NUM
// INODE_SIZE = 40
// 1 + 1 + (X / 1024) + (X / 1024) + (INODE_SIZE * X / 1024) + (X) = 16384
// (1 + 1 + 40 + 1024)[1066] * X / 1024 = 16382
// X = 15730
// 1 + 1 + 16 + 16 + 615 + 15730 = 16379
#pragma endregion

#define MAX_STORAGE_SIZE 16777216		// 最大磁盘空间 16 * 1024 * 1024 = 16777216
#define INODE_NUM 15730					// INODE数目
#define BLOCK_SIZE 1024					// BLOCK大小 = 1 KB
#define BLOCK_NUM 15730					// BLOCK数目
#define ADDRESS_LEN 3					// 地址长度 = 3 B
#define DIRECT_BLOCK_NUM 10				// 直接访问
#define INDIRECT_BLOCK_NUM 1			// 间接访问
#define DIRECTORY_SIZE 20				// 目录大小
#define FILE_NAME_LEN 20				// 文件名长度
#define SUPER_BLOCK_START 1				// 超级块起点
#define INODE_BITMAP_START 2			// INODE位图起点
#define BLOCK_BITMAP_START 18			// BLOCK位图起点
#define INODE_START 34					// INODE区起点
#define ROOT_DIRECTORY_START 649		// 根目录起点
#define STORAGE_START 650				// 文件和目录起点


#pragma region Struct
// 引导块 超级块 空闲空间管理 INODE 根目录 文件和目录
// 1      1      16 + 16      615   1       15730 
// 0      1      2    18      34    649     650
// size = 40
struct INODE
{
	ushort ino;										// INODE号
	ushort directBlock[DIRECT_BLOCK_NUM];			// 直接
	ushort indirectBlock[INDIRECT_BLOCK_NUM];		// 间接
	ushort links;									// 链接数
	uint size;										// 大小
	uint createTime;								// 时间
	int fmode;										// 文件类型 0 = 文件夹 1 = 文件
};
// 间接寻址块
struct IndirectionBlock
{
	ushort order;
	ushort nxtBlock[BLOCK_SIZE / ADDRESS_LEN];
};
// 超级块
struct SuperBlock
{
	ushort inodeNum;								// INODE总数
	ushort finodeNum;								// 空闲INODE总数
	ushort blockNum;								// BLOCK总数
	ushort fblockNum;								// 空闲BLOCK总数
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
	DirectoryElement item[DIRECTORY_SIZE];
};

SuperBlock superBlock;
bool inodeBitmap[INODE_NUM];
bool blockBitmap[BLOCK_NUM];
Directory curDirectory;
#pragma endregion

#pragma region Function

#pragma endregion


#pragma region Command
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
	printf("IndirectionBlock size = %d\n", sizeof(IndirectionBlock));
	printf("SuperBlock size = %d\n", sizeof(SuperBlock));
	printf("DirectoryElement size = %d\n", sizeof(DirectoryElement));
	printf("Directory size = %d\n", sizeof(Directory));
	printf("superBlock size = %d\n", sizeof(superBlock));
	printf("inodeBitmap size = %d\n", sizeof(inodeBitmap));
	printf("blockBitmap size = %d\n", sizeof(blockBitmap));
	printf("curDirectory size = %d\n", sizeof(curDirectory));
	uint test_A = -1;
	printf("%u\n", test_A);
	printf("%d\n", 16 * 1024);
	ushort test_B = -1;
	printf("%u\n", test_B);
	// end  //

	Welcome();
	char input[100];
	while (true)
	{
		scanf("%[^\n]", input); getchar();
		if (!parse(input)) break;
	}
}