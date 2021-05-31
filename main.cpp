#include <iostream>
#include <cstring>
#include <vector>
#include <cstdio>
#include <ctime>
using namespace std;
typedef unsigned int uint;
typedef unsigned short ushort;
const char* cmdHelp = "help";
const char* cmdCreateFile = "createFile";
const char* cmdDeleteFile = "deleteFile";
const char* cmdCreateDir = "createDir";
const char* cmdDeleteDir = "deleteDir";
const char* cmdChangeDir = "changeDir";
const char* cmdDir = "dir";
const char* cmdCp = "cp";
const char* cmdSum = "sum";
const char* cmdCat = "cat";
const char* cmdExit = "exit";
const char* STR_FILE_SYSTEM = "filesystem.mt";

#pragma region PreCal
// TOTAL = 16 MB 
// BLOCK_SIZE = 1 KB
// BLOCK_NUM = 16 * 1024 = (1<<14) = 16384
// 所以24b地址长度中只有14b有效，为方便起见，使用short模拟24b地址长度[-32768,32767]

// X = BLOCK_NUM = INODE_NUM
// INODE_SIZE = 40
// 1 + 1 + (X / 1024) + (X / 1024) + (INODE_SIZE * X / 1024) + (X) = 16384
// (1 + 1 + 40 + 1024)[1066] * X / 1024 = 16382
// X = 15730
// 1 + 1 + 16 + 16 + 615 + 15730 = 16379
#pragma endregion

const int MAX_STORAGE_SIZE = 16777216;							// 最大磁盘空间 16 * 1024 * 1024 = 16777216 B
const short INODE_NUM = 15730;									// INODE数目
const short BLOCK_SIZE = 1024;									// BLOCK大小 = 1 KB
const short BLOCK_NUM = 15730;									// BLOCK数目
const short ADDRESS_LEN = 3;									// 地址长度 = 3 B
const short DIRECT_BLOCK_NUM = 10;								// 直接访问
const short INDIRECT_BLOCK_NUM = 1;								// 间接访问
const short DIRECTORY_SIZE = 20;								// 目录大小
const short FILE_NAME_LEN = 20;									// 文件名长度
const short PATH_NAME_LEN = 400;								// 路径长度
const short MUL_INDIRECT_BLOCK_NUM = BLOCK_SIZE / ADDRESS_LEN;	// 间接块中有多少个块地址
const int SUPER_BLOCK_START = 1 * BLOCK_SIZE;					// 超级块起点
const int INODE_BITMAP_START = 2 * BLOCK_SIZE;					// INODE位图起点
const int BLOCK_BITMAP_START = 18 * BLOCK_SIZE;					// BLOCK位图起点
const int INODE_START = 34 * BLOCK_SIZE;						// INODE区起点
const int STORAGE_START = 649 * BLOCK_SIZE;						// 文件和目录起点
const int LIMIT_DIRECT_BLOCK = 10;								// 直接访问临界，需要的空间为 n
const int LIMIT_ONE_INDIRECT_BLOCK = 351;						// 一次间接访问临界 BLOCK_SIZE / ADDRESS_LEN + LIMIT_DIRECT_BLOCK = 351 需要的空间为 n+1 		
																// 二次间接访问临界 = 116291 KB > 16 MB 需要的空间为 n+1+ceil((n-10)/341) = n+1+(n-10+341-1)/341

#pragma region Struct
// 引导块 超级块 空闲空间管理 INODE 文件和目录
// 1      1      16 + 16      615    15730 
// 0      1      2    18      34     649

// size = 40
struct INODE
{
	short ino;											// INODE号
	short links;										// 链接数
	short size;											// 大小(KB)
	short directBlock[DIRECT_BLOCK_NUM];				// 直接
	short indirectBlock[INDIRECT_BLOCK_NUM];			// 间接
	int fmode;											// 文件类型 0 = 文件夹 1 = 文件
	time_t createTime;									// 时间
	void init()
	{
		ino = -1;
		links = 0;
		size = 0;
		memset(directBlock, -1, sizeof(directBlock));
		memset(indirectBlock, -1, sizeof(indirectBlock));
		fmode = -1;
		createTime = time(0);
	}
};
// 间接寻址块
struct IndirectionBlock
{
	short order;
	short nxtBlock[MUL_INDIRECT_BLOCK_NUM];
	void init()
	{
		order = 0;
		memset(nxtBlock, -1, sizeof(nxtBlock));
	}
};
// 超级块
struct SuperBlock
{
	short inodeNum;										// INODE总数
	short finodeNum;									// 空闲INODE总数
	short blockNum;										// BLOCK总数
	short fblockNum;									// 空闲BLOCK总数
};
// 文件夹元素
struct DirectoryElement
{
	char fileName[FILE_NAME_LEN];
	short ino;
};
// 文件夹
struct Directory
{
	DirectoryElement item[DIRECTORY_SIZE], father, self;
	short itemNum;
	short blockIdx;
	void Init()
	{
		itemNum = 0;
		self.ino = father.ino = blockIdx = -1;
		memset(father.fileName, 0, sizeof(father.fileName));
		memset(self.fileName, 0, sizeof(self.fileName));
		for (int i = 0; i < DIRECTORY_SIZE; i++)
		{
			memset(item[i].fileName, 0, sizeof(item[i].fileName));
			item[i].ino = -1;
		}
	}
};

SuperBlock superBlock;
bool inodeBitmap[INODE_NUM];							// 0 表示未使用
bool blockBitmap[BLOCK_NUM];							// 0 表示未使用
Directory curDirectory;									// 当前打开的文件夹
FILE* file;												// 文件系统
char randData[BLOCK_SIZE];								// 随机数据
char curPathName[PATH_NAME_LEN] = "/";					// 当前路径
#pragma endregion

#pragma region File_Function
// 读取对应idx的INODE
inline void ReadINODE(const short& idx, INODE& item);
// 写入对应idx的INODE
inline void WriteINODE(const short& idx, INODE& item);
// 读取超级块
inline void ReadSuperBlock(SuperBlock& item);
// 写入超级块
inline void WriteSuperBlock(SuperBlock& item);
// 读取inodeBitmap
inline void ReadBitmapINODE();
// 单点读取inodeBitmap
inline void ReadSingleBitmapINODE(const short& idx);
// 写入inodeBitmap
inline void WriteBitmapINODE();
// 单点写入inodeBitmap
inline void WriteSingleBitmapINODE(const short& idx);
// 读取blockBitmap
inline void ReadBitmapBlock();
// 单点读取blockBitmap
inline void ReadSingleBitmapBlock(const short& idx);
// 写入blockBitmap
inline void WriteBitmapBlock();
// 单点写入blockBitmap
inline void WriteSingleBitmapBlock(const short& idx);
// 读取间接块
inline void ReadIndirectionBlock(const short& idx, IndirectionBlock& item);
// 写入间接块
inline void WriteIndirectionBlock(const short& idx, IndirectionBlock& item);
// 读取Storage中的文件夹数据
inline void ReadDirectory(const short& idx, Directory& item);
// 写入Storage中的文件夹数据
inline void WriteDirectory(const short& idx, Directory& item);
// 读取Storage中的文件数据
inline void ReadStorageData(const short& idx, char* item);
// 写入Storage中的文件数据
inline void WriteStorageData(const short& idx, char* item);
#pragma endregion

#pragma region Op_Function
// char[] -> int
int str2int(char* str);
// 获取INODE的完整路径
char* CplPath(INODE item);
// 固定数量char输出
inline void putch(char ch, short number);
// 获取随机数据
char* GetRandData();
// 占用inode 更改bitmap、superBlock
inline void useINODE(const int& idx);
// 释放inode 更改bitmap、superBlock
inline void relINODE(const int& idx);
// 占用block 更改bitmap、superBlock
inline void useBlock(const int& idx);
// 释放block 更改bitmap、superBlock
inline void relBlock(const int& idx);
// 找到一个空闲INODE的idx 找不到返回-1
short FindFreeINODE();
// 找到是否有足够大小的block 否返回-1 否则返回第一个找到的block
short FindFreeBlock(const int& size);
// 分离路径，并返回INODE 
// 返回最后的INODE
bool Path2INODE(char* path, INODE& item);
// 分离路径，并返回INODE 
// 忽略最后的Name，返回前面的INODE，并且返回后面Name的char*
bool Path2INODE(char* path, INODE& item, char*& ret);
// 回退 SuperBlock,Bitmap
inline void RollBack();
// 删除对应idx的INODE
void DeleteINODE(const short& idx);
// 删除对应INODE
void DeleteINODE(INODE& item);
// 初始化，读入
bool Init();

#pragma endregion

#pragma region Command
// 启动载入磁盘
bool Start();
// 帮助信息
void Help();
// 欢迎信息
void WelcomeMsg();
// 启动
bool Welcome();
// 创建文件 createFile fileName fileSize (KB)
void CreateFile(char* fileName, char* strFileSize);
// 删除文件 deleteFile fileName
void DeleteFile(char* fileName);
// 创建文件夹 createDir dirName
void CreateDir(char* dirName);
// 删除文件夹 deleteDir dirName
void DeleteDir(char* dirName);
// 改变当前工作路径 changeDir path
void ChangeDir(char* path);
// 列出当前目录下的文件和子目录及对应信息(SIZE CREATETIME) dir
void Dir();
// 复制文件 cp file1 file2
// 暂时只实现文件复制
void Cp(char* fileName1, char* fileName2);
// 显示空间使用信息 sum
void Sum();
// 打印文件内容 cat fileName
void Cat(char* file);
// 退出
bool Exit();
// 解析并处理cmd 
// 退出返回0
bool Parse(char* cmd);
#pragma endregion

int main()
{
	//// test //
	//printf("INODE size = %d\n", sizeof(INODE));
	//printf("IndirectionBlock size = %d\n", sizeof(IndirectionBlock));
	//printf("SuperBlock size = %d\n", sizeof(SuperBlock));
	//printf("DirectoryElement size = %d\n", sizeof(DirectoryElement));
	//printf("Directory size = %d\n", sizeof(Directory));
	//printf("superBlock size = %d\n", sizeof(superBlock));
	//printf("inodeBitmap size = %d\n", sizeof(inodeBitmap));
	//printf("blockBitmap size = %d\n", sizeof(blockBitmap));
	//printf("curDirectory size = %d\n", sizeof(curDirectory));
	//printf("LIMIT_ONE_INDIRECT_BLOCK = %d\n", LIMIT_ONE_INDIRECT_BLOCK);
	//printf("LIMIT_DIRECT_BLOCK = %d\n", LIMIT_DIRECT_BLOCK);
	//printf("MUL_INDIRECT_BLOCK_NUM = %d\n", MUL_INDIRECT_BLOCK_NUM);
	//int test_A = -1;
	//printf("%d\n", test_A);
	//printf("%d\n", 16 * 1024);
	//short test_B = -1;
	//printf("%d\n", test_B);
	//printf("time_t size = %d\n", sizeof(time_t));
	//time_t tmpt = time(0);
	//printf("time_t = %s\n", ctime(&tmpt));
	//char test_str[40];
	//memset(test_str, 0, sizeof(test_str));
	//strcpy(test_str, "1231412412");
	//printf("str %s\n", test_str);
	//for (int i = ' '; i <= '~'; i++)
	//{
	//	printf("%c ", i);
	//}
	//printf("%d\n", '~' - ' ');
	//printf("\n");
	////////////
	//char strrr[] = "123";
	//printf("%s\n", strcat(strrr, "456"));
	//// end  //

	if (Welcome())
	{
		char input[PATH_NAME_LEN];
		while (true)
		{
			printf("%s# ", curPathName);
			scanf("%[^\n]", input); getchar();
			if (!Parse(input)) break;
		}
	}
}

#pragma region File_Function
// 读取对应idx的INODE
inline void ReadINODE(const short& idx, INODE& item)
{
	file = fopen(STR_FILE_SYSTEM, "rb+");
	fseek(file, INODE_START + idx * sizeof(INODE), 0);
	fread(&item, sizeof(INODE), 1, file);
	fclose(file);
}
// 写入对应idx的INODE
inline void WriteINODE(const short& idx, INODE& item)
{
	file = fopen(STR_FILE_SYSTEM, "rb+");
	fseek(file, INODE_START + idx * sizeof(INODE), 0);
	fwrite(&item, sizeof(INODE), 1, file);
	fclose(file);
}
// 读取超级块
inline void ReadSuperBlock(SuperBlock& item)
{
	file = fopen(STR_FILE_SYSTEM, "rb+");
	fseek(file, SUPER_BLOCK_START, 0);
	fread(&item, sizeof(SuperBlock), 1, file);
	fclose(file);
}
// 写入超级块
inline void WriteSuperBlock(SuperBlock& item)
{
	file = fopen(STR_FILE_SYSTEM, "rb+");
	fseek(file, SUPER_BLOCK_START, 0);
	fwrite(&item, sizeof(SuperBlock), 1, file);
	fclose(file);
}
// 读取inodeBitmap
inline void ReadBitmapINODE()
{
	file = fopen(STR_FILE_SYSTEM, "rb+");
	fseek(file, INODE_BITMAP_START, 0);
	fread(inodeBitmap, sizeof(inodeBitmap), 1, file);
	fclose(file);
}
// 单点读取inodeBitmap
inline void ReadSingleBitmapINODE(const short& idx)
{
	file = fopen(STR_FILE_SYSTEM, "rb+");
	fseek(file, INODE_BITMAP_START + idx * sizeof(bool), 0);
	fread(inodeBitmap + idx, sizeof(bool), 1, file);
	fclose(file);
}
// 写入inodeBitmap
inline void WriteBitmapINODE()
{
	file = fopen(STR_FILE_SYSTEM, "rb+");
	fseek(file, INODE_BITMAP_START, 0);
	fwrite(inodeBitmap, sizeof(inodeBitmap), 1, file);
	fclose(file);
}
// 单点写入inodeBitmap
inline void WriteSingleBitmapINODE(const short& idx)
{
	file = fopen(STR_FILE_SYSTEM, "rb+");
	fseek(file, INODE_BITMAP_START + idx * sizeof(bool), 0);
	fwrite(inodeBitmap + idx, sizeof(bool), 1, file);
	fclose(file);
}
// 读取blockBitmap
inline void ReadBitmapBlock()
{
	file = fopen(STR_FILE_SYSTEM, "rb+");
	fseek(file, BLOCK_BITMAP_START, 0);
	fread(blockBitmap, sizeof(blockBitmap), 1, file);
	fclose(file);
}
// 单点读取blockBitmap
inline void ReadSingleBitmapBlock(const short& idx)
{
	file = fopen(STR_FILE_SYSTEM, "rb+");
	fseek(file, BLOCK_BITMAP_START + idx * sizeof(bool), 0);
	fread(blockBitmap + idx, sizeof(bool), 1, file);
	fclose(file);
}
// 写入blockBitmap
inline void WriteBitmapBlock()
{
	file = fopen(STR_FILE_SYSTEM, "rb+");
	fseek(file, BLOCK_BITMAP_START, 0);
	fwrite(blockBitmap, sizeof(blockBitmap), 1, file);
	fclose(file);
}
// 单点写入blockBitmap
inline void WriteSingleBitmapBlock(const short& idx)
{
	file = fopen(STR_FILE_SYSTEM, "rb+");
	fseek(file, BLOCK_BITMAP_START + idx * sizeof(bool), 0);
	fwrite(blockBitmap + idx, sizeof(bool), 1, file);
	fclose(file);
}
// 读取间接块
inline void ReadIndirectionBlock(const short& idx, IndirectionBlock& item)
{
	file = fopen(STR_FILE_SYSTEM, "rb+");
	fseek(file, STORAGE_START + idx * BLOCK_SIZE, 0);
	fread(&item, sizeof(IndirectionBlock), 1, file);
	fclose(file);
}
// 写入间接块
inline void WriteIndirectionBlock(const short& idx, IndirectionBlock& item)
{
	file = fopen(STR_FILE_SYSTEM, "rb+");
	fseek(file, STORAGE_START + idx * BLOCK_SIZE, 0);
	fwrite(&item, sizeof(IndirectionBlock), 1, file);
	fclose(file);
}
// 读取Storage中的文件夹数据
inline void ReadDirectory(const short& idx, Directory& item)
{
	file = fopen(STR_FILE_SYSTEM, "rb+");
	fseek(file, STORAGE_START + idx * BLOCK_SIZE, 0);
	fread(&item, sizeof(Directory), 1, file);
	fclose(file);
}
// 写入Storage中的文件夹数据
inline void WriteDirectory(const short& idx, Directory& item)
{
	file = fopen(STR_FILE_SYSTEM, "rb+");
	fseek(file, STORAGE_START + idx * BLOCK_SIZE, 0);
	fwrite(&item, sizeof(Directory), 1, file);
	fclose(file);
}
// 读取Storage中的文件数据
inline void ReadStorageData(const short& idx, char* item)
{
	file = fopen(STR_FILE_SYSTEM, "rb+");
	fseek(file, STORAGE_START + idx * BLOCK_SIZE, 0);
	fread(item, BLOCK_SIZE, 1, file);
	fclose(file);
}
// 写入Storage中的文件数据
inline void WriteStorageData(const short& idx, char* item)
{
	file = fopen(STR_FILE_SYSTEM, "rb+");
	fseek(file, STORAGE_START + idx * BLOCK_SIZE, 0);
	fwrite(item, BLOCK_SIZE, 1, file);
	fclose(file);
}
#pragma endregion

#pragma region Op_Function
// char[] -> int
int str2int(char* str)
{
	int ret = 0;
	for (int i = 0; str[i]; i++)
	{
		if (str[i] >= '0' && str[i] <= '9')
		{
			ret = ret * 10 + str[i] - '0';
		}
		else
		{
			printf("数字非法\n");
			return -1;
		}
		if (ret > superBlock.fblockNum)
		{
			printf("Block空间不足\n");
			return -1;
		}
	}
	return ret;
}
// 获取INODE的完整路径
char* CplPath(INODE item)
{
	Directory dirTmp;
	ReadDirectory(item.directBlock[0], dirTmp);
	if (dirTmp.father.ino == dirTmp.self.ino)
	{
		curPathName[0] = '\0';
		return curPathName;
	}
	ReadINODE(dirTmp.father.ino, item);
	return strcat(strcat(CplPath(item),"/"), dirTmp.self.fileName);
}
// 固定数量char输出
inline void putch(char ch, short number)
{
	while (number--) putchar(ch);
}
// 获取随机数据
char* GetRandData()
{
	for (int i = 0; i < BLOCK_SIZE; i++) randData[i] = rand() % 95 + ' ';
	return randData;
}
// 占用inode 更改bitmap、superBlock
inline void useINODE(const int& idx)
{
	if (inodeBitmap[idx] == 0)
	{
		inodeBitmap[idx] = 1;
		superBlock.finodeNum -= 1;
	}
}
// 释放inode 更改bitmap、superBlock
inline void relINODE(const int& idx)
{
	if (inodeBitmap[idx] == 1)
	{
		inodeBitmap[idx] = 0;
		superBlock.finodeNum += 1;
	}
}
// 占用block 更改bitmap、superBlock
inline void useBlock(const int& idx)
{
	if (blockBitmap[idx] == 0)
	{
		blockBitmap[idx] = 1;
		superBlock.fblockNum -= 1;
	}
}
// 释放block 更改bitmap、superBlock
inline void relBlock(const int& idx)
{
	if (blockBitmap[idx] == 1)
	{
		blockBitmap[idx] = 0;
		superBlock.fblockNum += 1;
	}
}
// 找到一个空闲INODE的idx 找不到返回-1
short FindFreeINODE()
{
	static short idx = 0;
	if (superBlock.finodeNum <= 0) return -1;
	while (inodeBitmap[idx] != 0)
	{
		idx++;
		if (idx >= INODE_NUM) idx -= INODE_NUM;
	}
	return idx++;
}
// 找到是否有足够大小的block 否返回-1 否则返回第一个找到的block
// size(KB == BLOCK_NUM)
short FindFreeBlock(const int& size)
{
	static short idx = 0;
	if (size <= 0) return -1;
	if (superBlock.fblockNum < size) return -1;
	while (blockBitmap[idx] != 0)
	{
		idx++;
		if (idx >= BLOCK_NUM) idx -= BLOCK_NUM;
	}
	return idx++;
}
// 分离路径，并返回INODE 
// 引用返回最后的INODE，无返回值
bool Path2INODE(char* path, INODE& item)
{
	Directory dirTmp;
	dirTmp = curDirectory;
	if (path[0] == '/') // 回到根节点
	{
		ReadDirectory(0, dirTmp);
		path += 1;
	}
	short len = strlen(path);
	if (path[len - 1] == '/' || path[0] == '/')
	{
		printf("格式错误\n");
		return 0;
	}
	short inoIdx;
	short i = 0, j = 0;
	while (i < len && j < len)
	{
		while (path[i] != '/' && path[i] != '\0') i++;
		while (path[j] == '/' || path[j] == '\0') j++;
		while (i < len && (path[i] == '/' || path[i] == '\0'))
		{
			path[i] = '\0';
			i++;
		}
		inoIdx = -1;
		for (int k = 0; k < DIRECTORY_SIZE; k++)
		{
			if (strcmp(dirTmp.item[k].fileName, path + j) == 0)
			{
				inoIdx = dirTmp.item[k].ino;
				break;
			}
		}
		if (inoIdx == -1)
		{
			printf("不存在对应文件%s\n", path + j);
			return 0;
		}
		ReadINODE(inoIdx, item);
		if (i >= len)
		{
			return 1;
		}
		else
		{
			if (item.fmode == 1)
			{
				printf("%s是一个文件\n", path + j);
				return 0;
			}
			else
			{
				ReadDirectory(item.directBlock[0], dirTmp);
			}
		}
		j = i;
	}
	return 1;
}
// 分离路径，并返回INODE 
// 忽略最后的Name，返回前面的INODE，并且返回后面Name的char*
bool Path2INODE(char* path, INODE& item, char*& ret)
{
	Directory dirTmp;
	dirTmp = curDirectory;
	if (path[0] == '/') // 回到根节点
	{
		ReadDirectory(0, dirTmp);
		path += 1;
	}
	short len = strlen(path);
	if (path[len - 1] == '/' || path[0] == '/')
	{
		printf("格式错误\n");
		return 0;
	}
	short i = len - 1;
	while (i > 0)
	{
		if (path[i] == '/') break;
		i--;
	}
	if (i == 0)
	{
		ReadINODE(dirTmp.self.ino, item);
		ret = path;
		return 1;
	}
	ret = path + i + 1;
	while (i > 0 && path[i] == '/') i--;
	len = i + 1;
	path[len] = '\0';
	short inoIdx, j;
	i = j = 0;
	while (i < len && j < len)
	{
		while (path[i] != '/' && path[i] != '\0') i++;
		while (path[j] == '/' || path[j] == '\0') j++;
		while (i < len && (path[i] == '/' || path[i] == '\0'))
		{
			path[i] = '\0';
			i++;
		}
		inoIdx = -1;
		for (int k = 0; k < DIRECTORY_SIZE; k++)
		{
			if (strcmp(dirTmp.item[k].fileName, path + j) == 0)
			{
				inoIdx = dirTmp.item[k].ino;
				break;
			}
		}
		if (inoIdx == -1)
		{
			printf("不存在对应文件%s\n", path + j);
			return 0;
		}
		ReadINODE(inoIdx, item);
		if (i >= len)
		{
			return 1;
		}
		else
		{
			if (item.fmode == 1)
			{
				printf("%s是一个文件\n", path + j);
				return 0;
			}
			else
			{
				ReadDirectory(item.directBlock[0], dirTmp);
			}
		}
		j = i;
	}
}
// 回退 SuperBlock,Bitmap
inline void RollBack()
{
	ReadSuperBlock(superBlock);
	short idx = curDirectory.blockIdx;
	ReadDirectory(idx, curDirectory);
	ReadBitmapINODE();
	ReadBitmapBlock();
}
// 删除对应idx的INODE
void DeleteINODE(const short& idx)
{
	INODE inodeTmp;
	ReadINODE(idx, inodeTmp);
	DeleteINODE(inodeTmp);
}
// 删除对应INODE
void DeleteINODE(INODE& item)
{
	item.links--;
	if (item.links > 0) return;
	// 无链接删除
	if (item.fmode == 0)
	{
		INODE inodeTmp;
		Directory dirTmp;
		ReadDirectory(item.directBlock[0], dirTmp);
		for (int i = 0; i < DIRECTORY_SIZE; i++)
		{
			if (dirTmp.item[i].ino != -1)
			{
				DeleteINODE(dirTmp.item[i].ino);
			}
		}
		relINODE(item.ino);
		if (item.directBlock[0] == -1)
		{
			printf("数据错误\n");
			return;
		}
		relBlock(item.directBlock[0]);
		WriteSuperBlock(superBlock);
		WriteBitmapINODE();
		WriteBitmapBlock();
	}
	else if (item.fmode == 1)
	{
		for (int i = 0; i < DIRECT_BLOCK_NUM; i++)
		{
			if (item.directBlock[i] != -1)
			{
				relBlock(item.directBlock[i]);
				item.directBlock[i] = -1;
			}
		}
		for (int i = 0; i < INDIRECT_BLOCK_NUM; i++)
		{
			if (item.indirectBlock[i] != -1)
			{
				IndirectionBlock one;
				ReadIndirectionBlock(item.indirectBlock[i], one);
				if (one.order == 1)
				{
					IndirectionBlock two;
					for (int j = 0; j < MUL_INDIRECT_BLOCK_NUM; j++)
					{
						if (one.nxtBlock[j] != -1)
						{
							ReadIndirectionBlock(one.nxtBlock[j], two);
							for (int k = 0; k < MUL_INDIRECT_BLOCK_NUM; k++)
							{
								if (two.nxtBlock[k] != -1)
								{
									relBlock(two.nxtBlock[k]);
									two.nxtBlock[k] = -1;
								}
							}
							relBlock(one.nxtBlock[j]);
							one.nxtBlock[j] = -1;
						}
					}
				}
				else if (one.order == 0)
				{
					for (int j = 0; j < MUL_INDIRECT_BLOCK_NUM; j++)
					{
						if (one.nxtBlock[j] != -1)
						{
							relBlock(one.nxtBlock[j]);
							one.nxtBlock[j] = -1;
						}
					}
				}
				else
				{
					printf("间接块回收错误，order = %d\n", one.order);
				}
				relBlock(item.indirectBlock[i]);
				item.indirectBlock[i] = -1;
			}
		}
		relINODE(item.ino);
		WriteSuperBlock(superBlock);
		WriteBitmapINODE();
		WriteBitmapBlock();
	}
	else
	{
		printf("未知错误fmode\n");
		return;
	}
}
// 初始化，读入
bool Init()
{
	srand(time(0));
	file = fopen(STR_FILE_SYSTEM, "r");
	if (file == NULL)
	{
		printf("未找到对应文件，正在创建并初始化...\n");
		file = fopen(STR_FILE_SYSTEM, "wb");
		superBlock.inodeNum = superBlock.finodeNum = INODE_NUM;
		superBlock.blockNum = superBlock.fblockNum = BLOCK_NUM;
		memset(inodeBitmap, 0, sizeof(inodeBitmap));
		memset(blockBitmap, 0, sizeof(blockBitmap));
		short inoIdx = FindFreeINODE();
		if (inoIdx == -1)
		{
			printf("INODE空间不足，初始化失败\n");
			return 0;
		}
		short blockIdx = FindFreeBlock(1);
		if (blockIdx == -1)
		{
			printf("BLOCK空间不足，初始化失败\n");
			return 0;
		}
		curDirectory.Init();
		curDirectory.self.ino = inoIdx;
		curDirectory.father.ino = inoIdx;
		curDirectory.blockIdx = blockIdx;
		strcpy(curDirectory.self.fileName, "");
		INODE inode;
		inode.init();
		inode.fmode = 0;
		inode.ino = inoIdx;
		inode.directBlock[0] = blockIdx;
		inode.links++;
		inode.size = 1;
		inode.createTime = time(0);
		useINODE(inoIdx);
		useBlock(blockIdx);
		fseek(file, SUPER_BLOCK_START, 0);
		fwrite(&superBlock, sizeof(SuperBlock), 1, file);
		fseek(file, INODE_BITMAP_START, 0);
		fwrite(inodeBitmap, sizeof(inodeBitmap), 1, file);
		fseek(file, BLOCK_BITMAP_START, 0);
		fwrite(blockBitmap, sizeof(blockBitmap), 1, file);
		fseek(file, INODE_START + inoIdx * sizeof(INODE), 0);
		fwrite(&inode, sizeof(INODE), 1, file);
		fseek(file, STORAGE_START + blockIdx * BLOCK_SIZE, 0);
		fwrite(&curDirectory, sizeof(Directory), 1, file);
		printf("初始化完毕\n");
		//fclose(file);
	}
	else
	{
		fclose(file);
	}
	//file = fopen(STR_FILE_SYSTEM, "rb+");
	printf("正在读入数据\n");
	ReadSuperBlock(superBlock);
	ReadBitmapINODE();
	ReadBitmapBlock();
	ReadDirectory(0, curDirectory);
	printf("读入数据完毕\n");
	return 1;
}

#pragma endregion

#pragma region Command
// 启动载入磁盘
bool Start()
{
	printf("开始载入...\n");
	if (Init())
	{
		printf("载入完成\n");
		return 1;
	}
	else
	{
		printf("载入失败，退出系统\n");
		return 0;
	}
}
// 帮助信息
void Help()
{
	putch('*', 110); putchar('\n');
	printf("%-5s%-50s%-10s%-44s*\n", "*", "createFile [fileName] [fileSize(KB)]", "-----", "创建文件");
	printf("%-5s%-50s%-10s%-44s*\n", "*", "deleteFile [fileName]", "-----", "删除文件");
	printf("%-5s%-50s%-10s%-44s*\n", "*", "createDir [dirName]", "-----", "创建文件夹");
	printf("%-5s%-50s%-10s%-44s*\n", "*", "deleteDir [dirName]", "-----", "删除文件夹");
	printf("%-5s%-50s%-10s%-44s*\n", "*", "changeDir [path]", "-----", "改变当前工作路径");
	printf("%-5s%-50s%-10s%-44s*\n", "*", "dir", "-----", "列出当前目录下的文件和子目录及对应信息");
	printf("%-5s%-50s%-10s%-44s*\n", "*", "cp [file1] [file2]", "-----", "复制文件");
	printf("%-5s%-50s%-10s%-44s*\n", "*", "sum", "-----", "显示空间使用信息");
	printf("%-5s%-50s%-10s%-44s*\n", "*", "cat [fileName]", "-----", "打印文件内容");
	printf("%-5s%-50s%-10s%-44s*\n", "*", "exit", "-----", "退出");
	putch('*', 110); putchar('\n');
}
// 欢迎信息
void WelcomeMsg()
{
	printf("* *************************** Chinese Group 16 *************************** *\n");
	printf("*                                                                          *\n");
	printf("*                     Welcome to our Unix file system!                     *\n");
	printf("*                                                                          *\n");
	printf("*         Members:                                                         *\n");
	printf("*                          黄宗达 -- 201830570149                          *\n");
	printf("*                          徐自华 -- 201830570354                          *\n");
	printf("*                          王  葳 -- 201830570316                          *\n");
	printf("*                                                                          *\n");
	printf("* ************************************************************************ *\n\n");

	Help();
}
// 启动
bool Welcome()
{
	if (!Start())
	{
		return 0;
	}
	WelcomeMsg();
	return 1;
}
// 创建文件 createFile fileName fileSize (KB)
void CreateFile(char* fileName, char* strFileSize)
{
	if (superBlock.finodeNum <= 0)
	{
		printf("INODE空间不足\n");
		return;
	}
	short fileSize = str2int(strFileSize);
	if (fileSize == -1) return;
	if (fileSize == 0)
	{
		printf("请输入正整数\n");
		return;
	}
	// fileSize -> blockSize
	short blockSize = fileSize;
	short indirectType = 0;
	if (fileSize > LIMIT_ONE_INDIRECT_BLOCK)
	{
		blockSize = fileSize + 1 + (fileSize - DIRECT_BLOCK_NUM + MUL_INDIRECT_BLOCK_NUM - 1) / MUL_INDIRECT_BLOCK_NUM;
		indirectType = 2;
	}
	else if (fileSize > LIMIT_DIRECT_BLOCK)
	{
		blockSize = fileSize + 1;
		indirectType = 1;
	}
	if (blockSize > superBlock.fblockNum)
	{
		printf("Block空间不足\n");
		return;
	}
	INODE inode;
	if (Path2INODE(fileName, inode, fileName) == 0)
	{
		return;
	}
	if (strlen(fileName) > PATH_NAME_LEN)
	{
		printf("文件名过长，最大长度为20\n");
		return;
	}
	Directory dirTmp;
	if (inode.ino == curDirectory.self.ino)
	{
		dirTmp = curDirectory;
	}
	else
	{
		ReadDirectory(inode.directBlock[0], dirTmp);
	}
	// 检查是否重名
	for (int i = 0; i < DIRECTORY_SIZE; i++)
	{
		if (strcmp(fileName, dirTmp.item[i].fileName) == 0)
		{
			printf("无法创建重名文件\n");
			return;
		}
	}
	// 分配INODE
	if (dirTmp.itemNum >= DIRECTORY_SIZE)
	{
		printf("文件夹已满上限(20)\n");
		return;
	}
	short inoIdx = FindFreeINODE();
	if (inoIdx == -1)
	{
		printf("INODE空间不足\n");
		return;
	}
	inode.init();
	inode.ino = inoIdx;
	inode.fmode = 1;
	inode.links++;
	inode.size = fileSize;
	inode.createTime = time(0);
	for (int i = 0; i < DIRECTORY_SIZE; i++)
	{
		if (dirTmp.item[i].ino == -1)
		{
			dirTmp.itemNum++;
			strcpy(dirTmp.item[i].fileName, fileName);
			dirTmp.item[i].ino = inoIdx;
			break;
		}
	}
	useINODE(inoIdx);
	// 分配Block
	if (indirectType == 0)
	{
		for (int i = 0; i < inode.size; i++)
		{
			inode.directBlock[i] = FindFreeBlock(fileSize--);
			if (inode.directBlock[i] == -1)
			{
				printf("Block空间不足，已回退\n");
				RollBack();
				return;
			}
			WriteStorageData(inode.directBlock[i], GetRandData());
			useBlock(inode.directBlock[i]);
		}
	}
	else if (indirectType == 1)
	{
		for (int i = 0; i < DIRECT_BLOCK_NUM; i++)
		{
			inode.directBlock[i] = FindFreeBlock(fileSize--);
			if (inode.directBlock[i] == -1)
			{
				printf("Block空间不足，已回退\n");
				RollBack();
				return;
			}
			WriteStorageData(inode.directBlock[i], GetRandData());
			useBlock(inode.directBlock[i]);
		}
		for (int i = 0; i < INDIRECT_BLOCK_NUM && fileSize > 0; i++)
		{
			inode.indirectBlock[i] = FindFreeBlock(fileSize);
			if (inode.indirectBlock[i] == -1)
			{
				printf("Block空间不足，已回退\n");
				RollBack();
				return;
			}
			useBlock(inode.indirectBlock[i]);
			IndirectionBlock one;
			one.init();
			one.order = 0;
			for (int j = 0; j < MUL_INDIRECT_BLOCK_NUM && fileSize > 0; j++)
			{
				one.nxtBlock[j] = FindFreeBlock(fileSize--);
				if (one.nxtBlock[j] == -1)
				{
					printf("Block空间不足，已回退\n");
					RollBack();
					return;
				}
				WriteStorageData(one.nxtBlock[j], GetRandData());
				useBlock(one.nxtBlock[j]);
			}
			WriteIndirectionBlock(inode.indirectBlock[i], one);
		}
	}
	else if (indirectType == 2)
	{
		for (int i = 0; i < DIRECT_BLOCK_NUM; i++)
		{
			inode.directBlock[i] = FindFreeBlock(fileSize--);
			if (inode.directBlock[i] == -1)
			{
				printf("Block空间不足，已回退\n");
				RollBack();
				return;
			}
			WriteStorageData(inode.directBlock[i], GetRandData());
			useBlock(inode.directBlock[i]);
		}
		for (int i = 0; i < INDIRECT_BLOCK_NUM && fileSize > 0; i++)
		{
			inode.indirectBlock[i] = FindFreeBlock(fileSize);
			if (inode.indirectBlock[i] == -1)
			{
				printf("Block空间不足，已回退\n");
				RollBack();
				return;
			}
			useBlock(inode.indirectBlock[i]);
			IndirectionBlock one;
			one.init();
			one.order = 1;
			for (int j = 0; j < MUL_INDIRECT_BLOCK_NUM && fileSize > 0; j++)
			{
				one.nxtBlock[j] = FindFreeBlock(fileSize);
				if (one.nxtBlock[j] == -1)
				{
					printf("Block空间不足，已回退\n");
					RollBack();
					return;
				}
				useBlock(one.nxtBlock[j]);
				IndirectionBlock two;
				two.init();
				two.order = 0;
				for (int k = 0; k < MUL_INDIRECT_BLOCK_NUM && fileSize > 0; k++)
				{
					two.nxtBlock[k] = FindFreeBlock(fileSize--);
					if (two.nxtBlock[k] == -1)
					{
						printf("Block空间不足，已回退\n");
						RollBack();
						return;
					}
					WriteStorageData(two.nxtBlock[k], GetRandData());
					useBlock(two.nxtBlock[k]);
				}
				WriteIndirectionBlock(one.nxtBlock[j], two);
			}
			WriteIndirectionBlock(inode.indirectBlock[i], one);
		}
	}
	else
	{
		printf("未知错误，已回退\n");
		RollBack();
		return;
	}
	if (curDirectory.self.ino == dirTmp.self.ino)
	{
		curDirectory = dirTmp;
	}
	WriteDirectory(dirTmp.blockIdx, dirTmp);
	WriteINODE(inoIdx, inode);
	WriteSuperBlock(superBlock);
	WriteBitmapINODE();
	WriteBitmapBlock();
}
// 删除文件 deleteFile fileName
void DeleteFile(char* fileName)
{
	INODE inodeTmp;
	if (Path2INODE(fileName, inodeTmp, fileName) == 0)
	{
		return;
	}
	Directory dirTmp;
	if (curDirectory.self.ino == inodeTmp.ino)
	{
		dirTmp = curDirectory;
	}
	else
	{
		ReadDirectory(inodeTmp.directBlock[0], dirTmp);
	}
	int inoIdx = -1;
	for (int i = 0; i < DIRECTORY_SIZE; i++)
	{
		if (strcmp(fileName, dirTmp.item[i].fileName) == 0)
		{
			inoIdx = dirTmp.item[i].ino;
			break;
		}
	}
	if (inoIdx == -1)
	{
		printf("不存在对应文件\n");
		return;
	}
	ReadINODE(inoIdx, inodeTmp);
	if (inodeTmp.fmode == 0)
	{
		printf("对应文件为文件夹，请使用deleteDir %s\n", fileName);
		return;
	}
	for (int i = 0; i < DIRECTORY_SIZE; i++)
	{
		if (inoIdx == dirTmp.item[i].ino)
		{
			dirTmp.itemNum -= 1;
			dirTmp.item[i].ino = -1;
			memset(dirTmp.item[i].fileName, 0, sizeof(dirTmp.item[i].fileName));
			break;
		}
	}
	DeleteINODE(inodeTmp);
	if (curDirectory.self.ino == dirTmp.self.ino)
	{
		curDirectory = dirTmp;
	}
	WriteDirectory(dirTmp.blockIdx, dirTmp);
}
// 创建文件夹 createDir dirName
void CreateDir(char* dirName)
{
	if (superBlock.finodeNum <= 0)
	{
		printf("INODE空间不足\n");
		return;
	}
	INODE inode;
	if (Path2INODE(dirName, inode, dirName) == 0)
	{
		return;
	}
	if (strlen(dirName) > PATH_NAME_LEN)
	{
		printf("文件夹名过长，最大长度为20\n");
		return;
	}
	Directory dirTmp;
	if (curDirectory.self.ino == inode.ino)
	{
		dirTmp = curDirectory;
	}
	else
	{
		ReadDirectory(inode.directBlock[0], dirTmp);
	}
	for (int i = 0; i < DIRECTORY_SIZE; i++)
	{
		if (strcmp(dirName, dirTmp.item[i].fileName) == 0)
		{
			printf("无法创建重名文件\n");
			return;
		}
	}
	if (dirTmp.itemNum >= DIRECTORY_SIZE)
	{
		printf("文件夹已满上限(20)\n");
		return;
	}
	short inoIdx = FindFreeINODE();
	if (inoIdx == -1)
	{
		printf("INODE空间不足\n");
		return;
	}
	inode.init();
	inode.ino = inoIdx;
	inode.fmode = 0;
	inode.links++;
	inode.size = 1;
	inode.createTime = time(0);
	for (int i = 0; i < DIRECTORY_SIZE; i++)
	{
		if (dirTmp.item[i].ino == -1)
		{
			dirTmp.itemNum++;
			strcpy(dirTmp.item[i].fileName, dirName);
			dirTmp.item[i].ino = inoIdx;
			break;
		}
	}
	useINODE(inoIdx);
	inode.directBlock[0] = FindFreeBlock(1);
	if (inode.directBlock[0] == -1)
	{
		printf("Block空间不足，已回退\n");
		RollBack();
		return;
	}
	Directory dirTmp2;
	dirTmp2.Init();
	dirTmp2.father = dirTmp.self;
	strcpy(dirTmp2.self.fileName, dirName);
	dirTmp2.self.ino = inoIdx;
	dirTmp2.blockIdx = inode.directBlock[0];
	if (curDirectory.self.ino == dirTmp.self.ino)
	{
		curDirectory = dirTmp;
	}
	useBlock(inode.directBlock[0]);
	WriteDirectory(dirTmp.blockIdx, dirTmp);
	WriteDirectory(inode.directBlock[0], dirTmp2);
	WriteINODE(inoIdx, inode);
	WriteSuperBlock(superBlock);
	WriteBitmapINODE();
	WriteBitmapBlock();
}
// 删除文件夹 deleteDir dirName
void DeleteDir(char* dirName)
{
	int inoIdx = -1;
	INODE inodeTmp;
	if (Path2INODE(dirName, inodeTmp, dirName) == 0)
	{
		return;
	}
	Directory dirTmp;
	if (inodeTmp.ino == curDirectory.self.ino)
	{
		dirTmp = curDirectory;
	}
	else
	{
		ReadDirectory(inodeTmp.directBlock[0], dirTmp);
	}
	for (int i = 0; i < DIRECTORY_SIZE; i++)
	{
		if (strcmp(dirTmp.item[i].fileName, dirName) == 0)
		{
			inoIdx = dirTmp.item[i].ino;
			break;
		}
	}
	if (inoIdx == -1)
	{
		printf("不存在对应文件夹\n");
		return;
	}
	ReadINODE(inoIdx, inodeTmp);
	if (inodeTmp.fmode == 1)
	{
		printf("对应文件为文件，请使用deleteFile %s\n", dirName);
		return;
	}
	for (int i = 0; i < DIRECTORY_SIZE; i++)
	{
		if (inoIdx == dirTmp.item[i].ino)
		{
			dirTmp.itemNum -= 1;
			dirTmp.item[i].ino = -1;
			memset(dirTmp.item[i].fileName, 0, sizeof(dirTmp.item[i].fileName));
			break;
		}
	}
	DeleteINODE(inodeTmp);
	if (dirTmp.self.ino == curDirectory.self.ino)
	{
		curDirectory = dirTmp;
	}
	WriteDirectory(dirTmp.blockIdx, dirTmp);
}
// 改变当前工作路径 changeDir path
void ChangeDir(char* path)
{
	char* pathCp = path;
	if (strcmp(path, "..") == 0)
	{
		INODE inodeTmp;
		ReadINODE(curDirectory.father.ino, inodeTmp);
		ReadDirectory(inodeTmp.directBlock[0], curDirectory);
		CplPath(inodeTmp);
		if (curPathName[0] == '\0')
		{
			curPathName[0] = '/';
			curPathName[1] = '\0';
		}
		return;
	}
	else if (strcmp(path, ".") == 0)
	{
		return;
	}
	int inoIdx = -1;
	INODE inodeTmp;
	if (Path2INODE(path, inodeTmp) == 0)
	{
		return;
	}
	if (inodeTmp.fmode == 1)
	{
		printf("对应文件为文件\n");
		return;
	}
	ReadDirectory(inodeTmp.directBlock[0], curDirectory);
	CplPath(inodeTmp);
	if (curPathName[0] == '\0')
	{
		curPathName[0] = '/';
		curPathName[1] = '\0';
	}
}
// 列出当前目录下的文件和子目录及对应信息(SIZE CREATETIME) dir
void Dir()
{
	INODE inodeTmp;
	printf("%-20s %-20s %-20s %-30s\n", "Name", "Mode", "Size(KB)", "CreateTime");
	for (int i = 0; i < DIRECTORY_SIZE; i++)
	{
		if (curDirectory.item[i].ino != -1)
		{
			ReadINODE(curDirectory.item[i].ino, inodeTmp);
			if (inodeTmp.fmode == 0)
			{
				printf("%-20s %-20s %-20s %s", curDirectory.item[i].fileName, "directory", "", ctime(&inodeTmp.createTime));
			}
			else
			{
				printf("%-20s %-20s %-20d %s", curDirectory.item[i].fileName, "file", inodeTmp.size, ctime(&inodeTmp.createTime));
			}
		}
	}
}
// 复制文件 cp file1 file2
// 暂时只实现文件复制
void Cp(char* fileName1, char* fileName2)
{
	INODE inodeTmp1, inodeTmp2;
	short inoIdx1 = -1, inoIdx2 = -1;
	if (Path2INODE(fileName1, inodeTmp1, fileName1) == 0)
	{
		return;
	}
	if (Path2INODE(fileName2, inodeTmp2, fileName2) == 0)
	{
		return;
	}
	if (strlen(fileName2) > PATH_NAME_LEN)
	{
		printf("文件名过长，最大长度为20\n");
		return;
	}
	Directory dirTmp1, dirTmp2;
	if (curDirectory.self.ino == inodeTmp1.ino)
	{
		dirTmp1 = curDirectory;
	}
	else
	{
		ReadDirectory(inodeTmp1.directBlock[0], dirTmp1);
	}
	if (curDirectory.self.ino == inodeTmp2.ino)
	{
		dirTmp2 = curDirectory;
	}
	else
	{
		ReadDirectory(inodeTmp2.directBlock[0], dirTmp2);
	}
	// 处理fileName2创建问题
	for (int i = 0; i < DIRECTORY_SIZE; i++)
	{
		if (strcmp(fileName2, dirTmp2.item[i].fileName) == 0)
		{
			printf("无法创建重名文件\n");
			return;
		}
	}
	if (dirTmp2.itemNum >= DIRECTORY_SIZE)
	{
		printf("文件夹已满上限(20)\n");
		return;
	}
	inoIdx2 = FindFreeINODE();
	if (inoIdx2 == -1)
	{
		printf("INODE空间不足\n");
		return;
	}
	// 处理fileName1的INODE
	for (int i = 0; i < DIRECTORY_SIZE; i++)
	{
		if (strcmp(fileName1, dirTmp1.item[i].fileName) == 0)
		{
			inoIdx1 = dirTmp1.item[i].ino;
			break;
		}
	}
	if (inoIdx1 == -1)
	{
		printf("不存在文件%s\n", fileName1);
		return;
	}
	ReadINODE(inoIdx1, inodeTmp1);
	if (inodeTmp1.fmode == 0)
	{
		printf("文件%s是文件夹\n", fileName1);
		return;
	}
	short fileSize = inodeTmp1.size;
	// 判断是否超过大小
	short blockSize = fileSize, indirectType = 0;
	if (fileSize > LIMIT_ONE_INDIRECT_BLOCK)
	{
		blockSize = fileSize + 1 + (fileSize - DIRECT_BLOCK_NUM + MUL_INDIRECT_BLOCK_NUM - 1) / MUL_INDIRECT_BLOCK_NUM;
		indirectType = 2;
	}
	else if (fileSize > LIMIT_DIRECT_BLOCK)
	{
		blockSize = fileSize + 1;
		indirectType = 1;
	}
	if (blockSize > superBlock.fblockNum)
	{
		printf("Block空间不足\n");
		return;
	}
	// 开始分配INODE
	inodeTmp2.init();
	inodeTmp2.ino = inoIdx2;
	inodeTmp2.fmode = 1;
	inodeTmp2.links++;
	inodeTmp2.size = fileSize;
	inodeTmp2.createTime = time(0);
	for (int i = 0; i < DIRECTORY_SIZE; i++)
	{
		if (dirTmp2.item[i].ino == -1)
		{
			dirTmp2.itemNum++;
			strcpy(dirTmp2.item[i].fileName, fileName2);
			dirTmp2.item[i].ino = inoIdx2;
			break;
		}
	}
	useINODE(inoIdx2);
	// 分配Block
	if (indirectType == 0)
	{
		for (int i = 0; i < inodeTmp2.size; i++)
		{
			inodeTmp2.directBlock[i] = FindFreeBlock(fileSize--);
			if (inodeTmp2.directBlock[i] == -1)
			{
				printf("Block空间不足，已回退\n");
				RollBack();
				return;
			}
			ReadStorageData(inodeTmp1.directBlock[i], randData);
			WriteStorageData(inodeTmp2.directBlock[i], randData);
			useBlock(inodeTmp2.directBlock[i]);
		}
	}
	else if (indirectType == 1)
	{
		for (int i = 0; i < DIRECT_BLOCK_NUM; i++)
		{
			inodeTmp2.directBlock[i] = FindFreeBlock(fileSize--);
			if (inodeTmp2.directBlock[i] == -1)
			{
				printf("Block空间不足，已回退\n");
				RollBack();
				return;
			}
			ReadStorageData(inodeTmp1.directBlock[i], randData);
			WriteStorageData(inodeTmp2.directBlock[i], randData);
			useBlock(inodeTmp2.directBlock[i]);
		}
		for (int i = 0; i < INDIRECT_BLOCK_NUM && fileSize > 0; i++)
		{
			inodeTmp2.indirectBlock[i] = FindFreeBlock(fileSize);
			if (inodeTmp2.indirectBlock[i] == -1)
			{
				printf("Block空间不足，已回退\n");
				RollBack();
				return;
			}
			useBlock(inodeTmp2.indirectBlock[i]);
			IndirectionBlock one1, one2;
			one2.init();
			one2.order = 0;
			ReadIndirectionBlock(inodeTmp1.indirectBlock[i], one1);
			for (int j = 0; j < MUL_INDIRECT_BLOCK_NUM && fileSize > 0; j++)
			{
				one2.nxtBlock[j] = FindFreeBlock(fileSize--);
				if (one2.nxtBlock[j] == -1)
				{
					printf("Block空间不足，已回退\n");
					RollBack();
					return;
				}
				ReadStorageData(one1.nxtBlock[j], randData);
				WriteStorageData(one2.nxtBlock[j], randData);
				useBlock(one2.nxtBlock[j]);
			}
			WriteIndirectionBlock(inodeTmp2.indirectBlock[i], one2);
		}
	}
	else if (indirectType == 2)
	{
		for (int i = 0; i < DIRECT_BLOCK_NUM; i++)
		{
			inodeTmp2.directBlock[i] = FindFreeBlock(fileSize--);
			if (inodeTmp2.directBlock[i] == -1)
			{
				printf("Block空间不足，已回退\n");
				RollBack();
				return;
			}
			ReadStorageData(inodeTmp1.directBlock[i], randData);
			WriteStorageData(inodeTmp2.directBlock[i], randData);
			useBlock(inodeTmp2.directBlock[i]);
		}
		for (int i = 0; i < INDIRECT_BLOCK_NUM && fileSize > 0; i++)
		{
			inodeTmp2.indirectBlock[i] = FindFreeBlock(fileSize);
			if (inodeTmp2.indirectBlock[i] == -1)
			{
				printf("Block空间不足，已回退\n");
				RollBack();
				return;
			}
			useBlock(inodeTmp2.indirectBlock[i]);
			IndirectionBlock one1, one2;
			one2.init();
			one2.order = 1;
			ReadIndirectionBlock(inodeTmp1.indirectBlock[i], one1);
			for (int j = 0; j < MUL_INDIRECT_BLOCK_NUM && fileSize > 0; j++)
			{
				one2.nxtBlock[j] = FindFreeBlock(fileSize);
				if (one2.nxtBlock[j] == -1)
				{
					printf("Block空间不足，已回退\n");
					RollBack();
					return;
				}
				useBlock(one2.nxtBlock[j]);
				IndirectionBlock two1, two2;
				two2.init();
				two2.order = 0;
				ReadIndirectionBlock(one1.nxtBlock[j], two1);
				for (int k = 0; k < MUL_INDIRECT_BLOCK_NUM; k++)
				{
					two2.nxtBlock[k] = FindFreeBlock(fileSize--);
					if (two2.nxtBlock[k] == -1)
					{
						printf("Block空间不足，已回退\n");
						RollBack();
						return;
					}
					ReadStorageData(two1.nxtBlock[k], randData);
					WriteStorageData(two2.nxtBlock[k], randData);
					useBlock(two2.nxtBlock[k]);
				}
				WriteIndirectionBlock(one2.nxtBlock[j], two2);
			}
			WriteIndirectionBlock(inodeTmp2.indirectBlock[i], one2);
		}
	}
	else
	{
		printf("未知错误，已回退\n");
		RollBack();
		return;
	}
	// 更新fileName2的信息
	if (dirTmp2.self.ino == curDirectory.self.ino)
	{
		curDirectory = dirTmp2;
	}
	WriteDirectory(dirTmp2.blockIdx, dirTmp2);
	WriteINODE(inoIdx2, inodeTmp2);
	WriteSuperBlock(superBlock);
	WriteBitmapINODE();
	WriteBitmapBlock();
}
// 显示空间使用信息 sum
void Sum()
{
	printf("%-20s %-20s %-20s %-20s %-20s %-20s %-20s\n", "", "BootBlock(/Block)", "SuperBlock(/Block)", "InodeBitmap(/Block)", "BlockBitmap(/Block)", "INODE(/Block)", "Storage(/Block)");
	printf("%-20s %-20s %-20s %-20s %-20s %-20s %-20d\n", "Total", "1", "1", "16", "16", "615", BLOCK_NUM);
	printf("%-20s %-20s %-20s %-20s %-20s %-20d %-20d\n", "CurrentUse", "1", "1", "16", "16", ((superBlock.inodeNum - superBlock.finodeNum) * sizeof(INODE) + BLOCK_SIZE - 1) / BLOCK_SIZE, superBlock.blockNum - superBlock.fblockNum);
	printf("\n");
	printf("%-20s %-20s %-20s\n", "", "INODE(/Number)", "Storage(/Block)");
	printf("%-20s %-20d %-20d\n", "Total", INODE_NUM, BLOCK_NUM);
	printf("%-20s %-20d %-20d\n", "CurrentUse", superBlock.inodeNum - superBlock.finodeNum, superBlock.blockNum - superBlock.fblockNum);
	printf("%-20s %-20.8f %-20.8f\n", "UseRate(%)", double(superBlock.inodeNum - superBlock.finodeNum) / double(INODE_NUM), double(superBlock.blockNum - superBlock.fblockNum) / double(BLOCK_NUM));
}
// 打印文件内容 cat fileName
void Cat(char* fileName)
{
	INODE inodeTmp;
	short inoIdx = -1;
	if (Path2INODE(fileName, inodeTmp, fileName) == 0)
	{
		return;
	}
	Directory dirTmp;
	if (curDirectory.self.ino == inodeTmp.ino)
	{
		dirTmp = curDirectory;
	}
	else
	{
		ReadDirectory(inodeTmp.directBlock[0], dirTmp);
	}
	for (int i = 0; i < DIRECTORY_SIZE; i++)
	{
		if (strcmp(fileName, dirTmp.item[i].fileName) == 0)
		{
			inoIdx = dirTmp.item[i].ino;
			break;
		}
	}
	if (inoIdx == -1)
	{
		printf("不存在文件%s\n", fileName);
		return;
	}
	ReadINODE(inoIdx, inodeTmp);
	if (inodeTmp.fmode == 0)
	{
		printf("文件%s是文件夹\n", fileName);
		return;
	}
	short curSize = inodeTmp.size;
	if (inodeTmp.size > LIMIT_DIRECT_BLOCK)
	{
		for (int i = 0; i < DIRECT_BLOCK_NUM && curSize > 0; i++)
		{
			if (inodeTmp.directBlock[i] == -1)
			{
				printf("数据块缺失\n");
				return;
			}
			ReadStorageData(inodeTmp.directBlock[i], randData);
			for (int ii = 0; ii < BLOCK_SIZE; ii++) putchar(randData[ii]);
			curSize--;
		}
		IndirectionBlock one, two;
		for (int i = 0; i < INDIRECT_BLOCK_NUM && curSize > 0; i++)
		{
			if (inodeTmp.indirectBlock[i] == -1)
			{
				printf("数据块缺失\n");
				return;
			}
			ReadIndirectionBlock(inodeTmp.indirectBlock[i], one);
			if (one.order == 1)
			{
				for (int j = 0; j < MUL_INDIRECT_BLOCK_NUM && curSize > 0; j++)
				{
					if (one.nxtBlock[j] == -1)
					{
						printf("数据块缺失\n");
						return;
					}
					ReadIndirectionBlock(one.nxtBlock[j], two);
					for (int k = 0; k < MUL_INDIRECT_BLOCK_NUM && curSize > 0; k++)
					{
						if (two.nxtBlock[k] == -1)
						{
							printf("数据块缺失\n");
							return;
						}
						ReadStorageData(two.nxtBlock[k], randData);
						for (int ii = 0; ii < BLOCK_SIZE; ii++) putchar(randData[ii]);
						curSize--;
					}
				}
			}
			else if (one.order == 0)
			{
				for (int j = 0; j < MUL_INDIRECT_BLOCK_NUM && curSize > 0; j++)
				{
					if (one.nxtBlock[j] == -1)
					{
						printf("数据块缺失\n");
						return;
					}
					ReadStorageData(one.nxtBlock[j], randData);
					for (int ii = 0; ii < BLOCK_SIZE; ii++) putchar(randData[ii]);
					curSize--;
				}
			}
			else
			{
				printf("未知错误\n");
				return;
			}
		}
		putchar('\n');
	}
	else
	{
		for (int i = 0; i < DIRECT_BLOCK_NUM && curSize > 0; i++)
		{
			if (inodeTmp.directBlock[i] == -1)
			{
				printf("数据块缺失\n");
				return;
			}
			ReadStorageData(inodeTmp.directBlock[i], randData);
			for (int ii = 0; ii < BLOCK_SIZE; ii++) putchar(randData[ii]);
			curSize--;
		}
		putchar('\n');
	}
}
// 退出
bool Exit()
{
	if (file == NULL) return 1;
	else if (fclose(file) == 0) return 1;
	else return 0;
}
// 解析并处理cmd 
// 退出返回0
bool Parse(char* cmd)
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
	//for (const char* item : vec)
	//{
	//	printf("%s\n", item);
	//}
	if (vec.size() == 0) return 1; // 无内容返回
	if (strcmp(vec[0], cmdHelp) == 0)
	{
		Help();
	}
	else if (strcmp(vec[0], cmdCreateFile) == 0)
	{
		if (vec.size() != 3)
		{
			printf("%s命令参数数目错误\n", vec[0]);
		}
		else
		{
			CreateFile(vec[1], vec[2]);
		}
	}
	else if (strcmp(vec[0], cmdDeleteFile) == 0)
	{
		if (vec.size() != 2)
		{
			printf("%s命令参数数目错误\n", vec[0]);
		}
		else
		{
			DeleteFile(vec[1]);
		}
	}
	else if (strcmp(vec[0], cmdCreateDir) == 0)
	{
		if (vec.size() != 2)
		{
			printf("%s命令参数数目错误\n", vec[0]);
		}
		else
		{
			CreateDir(vec[1]);
		}
	}
	else if (strcmp(vec[0], cmdDeleteDir) == 0)
	{
		if (vec.size() != 2)
		{
			printf("%s命令参数数目错误\n", vec[0]);
		}
		else
		{
			DeleteDir(vec[1]);
		}
	}
	else if (strcmp(vec[0], cmdChangeDir) == 0)
	{
		if (vec.size() != 2)
		{
			printf("%s命令参数数目错误\n", vec[0]);
		}
		else
		{
			ChangeDir(vec[1]);
		}
	}
	else if (strcmp(vec[0], cmdDir) == 0)
	{
		if (vec.size() != 1)
		{
			printf("%s命令参数数目错误\n", vec[0]);
		}
		else
		{
			Dir();
		}
	}
	else if (strcmp(vec[0], cmdCp) == 0)
	{
		if (vec.size() != 3)
		{
			printf("%s命令参数数目错误\n", vec[0]);
		}
		else
		{
			Cp(vec[1], vec[2]);
		}
	}
	else if (strcmp(vec[0], cmdSum) == 0)
	{
		if (vec.size() != 1)
		{
			printf("%s命令参数数目错误\n", vec[0]);
		}
		else
		{
			Sum();
		}
	}
	else if (strcmp(vec[0], cmdCat) == 0)
	{
		if (vec.size() != 2)
		{
			printf("%s命令参数数目错误\n", vec[0]);
		}
		else
		{
			Cat(vec[1]);
		}
	}
	else if (strcmp(vec[0], cmdExit) == 0)
	{
		if (Exit())
		{
			printf("已退出系统\n");
		}
		return 0;
	}
	else
	{
		printf("无法识别命令%s\n", vec[0]);
	}
	return 1;
}
#pragma endregion
