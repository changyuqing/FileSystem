#include <iostream>
#include <iomanip>
#include <cstring>
#include <bitset>
#include <stack>
#include <ctime>

using namespace std;
using std::bitset;

struct DirName {
    char name[11];      //文件名或目录名
    unsigned char key;  //目录标志
    int inodeId;        //Inode编号

    DirName() {         //初始化结构体
        strcpy(name, "");
        key = '2';
        inodeId = -1;
    }
};

struct INode {              //Inode项
    unsigned char readonly; //只读
    unsigned char hidden;   //隐藏
    unsigned char hour;     //小时
    unsigned char minute;   //分钟
    int directAddress[2];   //直接盘块地址
    int indexedAddress;     //一级索引块
};

union DataBlock {       //数据块
    DirName dirName[4]; //目录文件数据块
    int index[16];      //索引块
    char data[64];      //文本文件数据块
    DataBlock() {
    }
};

struct Disk {
    DirName dir[4];             //根目录
    bitset<516> inodeBitMap;    //I-node位图
    bitset<1024> blockBitMap;   //数据块位图
    INode iNode[128];           //I-node
    DataBlock dataBlock[1024];  //数据块

    Disk() {
    }
};


char path[40] = "root"; //当前路径
Disk disk;              //虚拟磁盘
int dirNum;             //相应编号的目录项数
INode *currInode;       //当前目录的inode结构
stack<int> history;     //路径栈

void Format();

int findEmptyINode();

int findEmptyBlock();

void Viewinodemap();

void Viewblockmap();

void MakeFile(unsigned char);

void DeleteFile(unsigned char);

void listAll();

void change_path(char *);

void cd(char []);

void open(char []);

int findDir(char [], int, unsigned char);

void copy();

void attrib();


void Format() {
    for (int i = 0; i < history.size(); i++)
        history.pop();
    history.push(-1);
    for (int i = 0; i < 4; i++) {
        disk.dir[i].inodeId = -1;
    }
    disk.inodeBitMap.reset();
    disk.blockBitMap.reset();
}

int findEmptyINode() {
    int INodeId = -1;
    for (int i = 0; i < 512; ++i) {
        if (!disk.inodeBitMap.test(i)) {
            disk.inodeBitMap.set(i);
            INodeId = i;
            break;
        }
    }

    return INodeId;
}

int findEmptyBlock() {
    int BlockId = -1;
    for (int i = 0; i < 1024; ++i) {
        if (!disk.blockBitMap.test(i)) {
            disk.blockBitMap.set(i);
            BlockId = i;
            break;
        }
    }
    return BlockId;
}

void Viewinodemap() {
    for (int i = 0; i < 512; ++i) {
        cout << disk.inodeBitMap[i];
        if ((i + 1) % 8 == 0)
            cout << " ";
        if ((i + 1) % 32 == 0)
            cout << endl;
    }

}

void Viewblockmap() {
    for (int i = 0; i < 1024; ++i) {
        cout << disk.blockBitMap[i];
        if ((i + 1) % 8 == 0)
            cout << " ";
        if ((i + 1) % 32 == 0)
            cout << endl;
    }
}

void MakeFile(unsigned char key) {
    char name[40];
    cin >> name;
    if (strcmp(name, "..") == 0) {
        cout << "命名错误" << endl;
        return;
    }
    DirName *dirName;
//    if(checkFileName(name,key)){
    if (findDir(name, history.top(), key) == -2) {
        if (history.top() == -1) {
            for (int i = 0; i < 4; i++) {
                if (disk.dir[i].inodeId == -1) {
                    dirName = &disk.dir[i];
                    goto a;
                }
            }
        } else {

            for (int j = 0; j < 2; ++j) {
                for (int i = 0; i < 4; i++) {
                    if (j == 1 && currInode->directAddress[j] == -1) {
                        currInode->directAddress[j] = findEmptyBlock();
                        for (int i = 0; i < 4; i++)
                            disk.dataBlock[currInode->directAddress[j]].dirName[i].inodeId = -1;
                    }
                    dirName = &disk.dataBlock[currInode->directAddress[j]].dirName[i];
                    if (dirName->inodeId == -1)
                        goto a;
                }
            }
            if (currInode->indexedAddress == -1) {
                currInode->indexedAddress = findEmptyBlock();
                for (int i = 0; i < 16; i++)
                    disk.dataBlock[currInode->indexedAddress].index[i] = -1;
            }

            for (int i = 0; i < 16; i++) {
                if (disk.dataBlock[currInode->indexedAddress].index[i] == -1) {
                    disk.dataBlock[currInode->indexedAddress].index[i] = findEmptyBlock();
                    for (int j = 0; j < 4; ++j) {
                        disk.dataBlock[disk.dataBlock[currInode->indexedAddress].index[i]].dirName[j].inodeId = -1;
                    }
                }
                for (int j = 0; j < 4; ++j) {


                    dirName = &disk.dataBlock[disk.dataBlock[currInode->indexedAddress].index[i]].dirName[j];
                    if (dirName->inodeId == -1) {
                        goto a;
                    }

                }
            }

        }
        cout << "当前目录已满" << endl;
        return;
        a:
        strcpy(dirName->name, name);
        dirName->key = key;
        dirName->inodeId = findEmptyINode();

        disk.iNode[dirName->inodeId].readonly = '0';
        disk.iNode[dirName->inodeId].hidden = '0';
        // 基于当前系统的当前日期/时间
        time_t now = time(0);
        tm *ltm = localtime(&now);
        // 输出 tm 结构的各个组成部分
        disk.iNode[dirName->inodeId].hour = (unsigned char) (ltm->tm_hour);
        disk.iNode[dirName->inodeId].minute = (unsigned char) (ltm->tm_min);
        disk.iNode[dirName->inodeId].directAddress[0] = findEmptyBlock();
        if (key == '0') {
            for (int i = 0; i < 4; i++)
                disk.dataBlock[disk.iNode[dirName->inodeId].directAddress[0]].dirName[i].inodeId = -1;
        } else {
            disk.dataBlock[disk.iNode[dirName->inodeId].directAddress[0]].data[0] = '\0';
        }

        disk.iNode[dirName->inodeId].directAddress[1] = -1;
        disk.iNode[dirName->inodeId].indexedAddress = -1;
    } else cout << "文件或文件夹已存在" << endl;
}

void DeleteFile(unsigned char key) {
    char name[40];
    cin >> name;
    DirName *tempDir;
    if (history.top() == -1) {
        for (int i = 0; i < 4; i++) {
            if (disk.dir[i].key == key && strcmp(name, disk.dir[i].name) == 0) {

                tempDir = &disk.dir[i];
                goto a;
            }

        }
    } else {

        for (int j = 0; j < 2; ++j) {
            if (j == 1) {
                if (currInode->directAddress[j] == -1) {
                    cout << "文件不存在" << endl;
                    return;
                }

            }
            for (int i = 0; i < 4; i++) {
                tempDir = &disk.dataBlock[currInode->directAddress[j]].dirName[i];
                if (tempDir->key == key && strcmp(name, tempDir->name) == 0) {
                    goto a;
                }
            }
        }
        if (currInode->indexedAddress == -1) {
            cout << "文件不存在" << endl;
            return;
        }
        for (int i = 0; i < 16; i++) {
            if (disk.dataBlock[currInode->indexedAddress].index[i] == -1) {
                cout << "文件不存在" << endl;
                return;
            }
            for (int j = 0; j < 4; ++j) {
                tempDir = &disk.dataBlock[disk.dataBlock[currInode->indexedAddress].index[i]].dirName[j];
                if (tempDir->key == key && strcmp(name, tempDir->name) == 0) {
                    goto a;
                }
            }
        }

    }
    cout << "文件不存在" << endl;
    return;
    a:
    if (disk.iNode[tempDir->inodeId].readonly == '1') {
        cout << "文件只读" << endl;
        return;
    }
    if (tempDir->key == '0') {
        if (disk.dataBlock[disk.iNode[tempDir->inodeId].directAddress[0]].dirName[0].inodeId != -1) {

            cout << "文件夹不为空" << endl;
            return;
        }

    } else {
        if (disk.iNode[tempDir->inodeId].directAddress[1] != -1) {
            disk.blockBitMap.reset(disk.iNode[tempDir->inodeId].directAddress[1]);
            disk.iNode[tempDir->inodeId].directAddress[1] = -1;
        }
        if (disk.iNode[tempDir->inodeId].indexedAddress != -1) {
            for (int i = 0; i < 16; ++i) {
                if (disk.dataBlock[disk.iNode[tempDir->inodeId].indexedAddress].index[i] != -1) {
                    disk.blockBitMap.reset(disk.dataBlock[disk.iNode[tempDir->inodeId].indexedAddress].index[i]);
                    disk.dataBlock[disk.iNode[tempDir->inodeId].indexedAddress].index[i] = -1;
                }
            }
            disk.blockBitMap.reset(disk.iNode[tempDir->inodeId].indexedAddress);
            disk.iNode[tempDir->inodeId].indexedAddress = -1;

        }
    }
    disk.blockBitMap.reset(disk.iNode[tempDir->inodeId].directAddress[0]);
    disk.iNode[tempDir->inodeId].directAddress[0] = -1;
    disk.inodeBitMap.reset(tempDir->inodeId);
    tempDir->inodeId = -1;


}

void listAll() {
    INode *temp;
    if (history.top() == -1) {
        for (int i = 0; i < 4; ++i) {
            if (disk.dir[i].inodeId != -1) {
                temp = &disk.iNode[disk.dir[i].inodeId];
                if (temp->hidden == '0') {
                    cout << "\t" << disk.dir[i].name << "\t" << disk.dir[i].key << "\t";
                    cout << (temp->hour - ' ' + 32) << ":";
                    cout << (temp->minute - ' ' + 32) << endl;
                }
            }
        }
    } else {
        for (int i = 0; i < 2; ++i) {
            if (disk.iNode[history.top()].directAddress[i] == -1)
                return;
            for (int j = 0; j < 4; ++j) {
                DirName *dirName = &disk.dataBlock[disk.iNode[history.top()].directAddress[i]].dirName[j];
                if ((dirName->inodeId) != -1) {
                    temp = &disk.iNode[dirName->inodeId];
                    if (temp->hidden == '0') {
                        cout << "\t" << dirName->name << "\t" << dirName->key << "\t";
                        cout << (temp->hour - ' ' + 32) << ":";
                        cout << (temp->minute - ' ' + 32) << endl;
                    }
                }
            }
        }
        if (disk.iNode[history.top()].indexedAddress == -1)
            return;
        for (int i = 0; i < 16; i++) {
            if (disk.dataBlock[disk.iNode[history.top()].indexedAddress].index[i] == -1)
                return;
            for (int j = 0; j < 4; ++j) {
                DirName *dirName = &disk.dataBlock[disk.dataBlock[disk.iNode[history.top()].indexedAddress].index[i]].dirName[j];
                if (dirName->inodeId != -1) {
                    temp = &disk.iNode[dirName->inodeId];
                    if (temp->hidden == '0') {
                        cout << "\t" << dirName->name << "\t" << dirName->key << "\t";
                        cout << (temp->hour - ' ' + 32) << ":";
                        cout << (temp->minute - ' ' + 32) << endl;
                    }
                }
            }
        }

    }
}

void change_path(char *name) {
    int pos;
    if (strcmp(name, "..") == 0) {//进入上层目录，将最后一个'/'后的内容去掉
        pos = strlen(path) - 1;
        for (; pos >= 0; --pos) {
            if (path[pos] == '/') {
                path[pos] = '\0';
                break;
            }
        }
    } else {//否则在路径末尾添加子目录
        strcat(path, "/");
        strcat(path, name);
    }
    return;
}

void cd(char name[]) {
    if (strcmp(name, "..") == 0)
        if (history.top() == -1) {
            cout << "无法返回上层" << endl;
            return;
        } else {
            change_path(name);
            history.pop();
            if (history.top() != -1) {
                currInode = &disk.iNode[history.top()];
            }
            return;
        }

    int inodenum = findDir(name, history.top(), '0');
    if (inodenum == -2) {
        cout << "文件夹不存在" << endl;
        return;
    } else {
        change_path(name);
        history.push(inodenum);
        if (history.top() != -1) {
            currInode = &disk.iNode[history.top()];
        }
    }
}

void open(char name[]) {
    INode *tempINode;
    int inodenum = findDir(name, history.top(), '1');
    if (inodenum == -2) {
        cout << "文件不存在" << endl;
        return;
    }
    tempINode = &disk.iNode[inodenum];
    if (tempINode->readonly == '1') {
        cout << "文件只读" << endl;
        return;
    }

    system("cls");
    char tempString[1152];

    tempString[0] = disk.dataBlock[tempINode->directAddress[0]].data[0];
    int k = 0;
    while (tempString[k] != '\0') {
        if (k < 64) {
            tempString[k] = disk.dataBlock[tempINode->directAddress[0]].data[k];
        } else if (k < 128) {
            tempString[k] = disk.dataBlock[tempINode->directAddress[1]].data[k - 64];
        } else {
            tempString[k] = disk.dataBlock[disk.dataBlock[tempINode->indexedAddress].index[(k - 128) / 64]].data[
                    (k - 128) % 64];
        }
        k++;
    }
    if (tempINode->directAddress[1] != -1) {
        disk.blockBitMap.reset(tempINode->directAddress[1]);
        tempINode->directAddress[1] = -1;
    }
    if (tempINode->indexedAddress != -1) {
        for (int i = 0; i < 16; ++i) {
            if (disk.dataBlock[tempINode->indexedAddress].index[i] != -1) {
                disk.blockBitMap.reset(disk.dataBlock[tempINode->indexedAddress].index[i]);
                disk.dataBlock[tempINode->indexedAddress].index[i] = -1;
            }
        }
        disk.blockBitMap.reset(tempINode->indexedAddress);
        tempINode->indexedAddress = -1;

    }
    cout << "当前文本内容" << endl;
    cout << tempString << endl;
    cout << "请输入新的内容" << endl;
    char c;
    scanf("%s", tempString);
    int length = strlen(tempString);

    if (length > 64) {
        tempINode->directAddress[1] = findEmptyBlock();
        disk.dataBlock[tempINode->directAddress[1]].data[0] = '\0';
    }
    if (length > 128) {
        tempINode->indexedAddress = findEmptyBlock();
        for (int i = 0; i < 16; i++)
            disk.dataBlock[tempINode->indexedAddress].index[i] = -1;
    }
    for (int i = 0; i <= (length - 128) / 64 && length > 128; ++i) {
        disk.dataBlock[tempINode->indexedAddress].index[i] = findEmptyBlock();
        disk.dataBlock[disk.dataBlock[tempINode->indexedAddress].index[i]].data[0] = '\0';

    }


    for (int i = 0; i < 64 && i <= length; ++i) {
        disk.dataBlock[tempINode->directAddress[0]].data[i] = tempString[i];
    }
    for (int i = 64; i < 128 && i <= length; ++i) {
        disk.dataBlock[tempINode->directAddress[1]].data[i - 64] = tempString[i];
    }
    for (int j = 0; j < 16; ++j) {
        for (int i = 128 + j * 64; i < 192 + j * 64 && i <= length; ++i) {
            disk.dataBlock[disk.dataBlock[tempINode->indexedAddress].index[j]].data[i - 128 - j * 64] = tempString[i];
        }
    }
    system("cls");

}

int findDir(char name[], int inodeid, unsigned char key) {
    if (strcmp(name, "root") == 0) {
        return -1;
    }
    if (inodeid == -1) {

        for (int i = 0; i < 4; i++) {
            if (disk.dir[i].inodeId == -1) {
                continue;
            }
            if (disk.dir[i].key == key && strcmp(name, disk.dir[i].name) == 0) {
                return disk.dir[i].inodeId;
            }

        }
    } else {

        for (int j = 0; j < 2; ++j) {
            if (disk.iNode[inodeid].directAddress[j] == -1)
                continue;
            for (int i = 0; i < 4; i++) {
                DirName *dirName = &disk.dataBlock[disk.iNode[inodeid].directAddress[j]].dirName[i];
                if (dirName->inodeId == -1) {
                    continue;
                }

                if (dirName->key == key && strcmp(name, dirName->name) == 0) {
                    return dirName->inodeId;
                }
            }
        }
        if (disk.iNode[inodeid].indexedAddress == -1) {
            return -2;
        }
        for (int i = 0; i < 16; i++) {
            if (disk.dataBlock[disk.iNode[inodeid].indexedAddress].index[i] == -1)
                continue;
            for (int j = 0; j < 4; ++j) {
                DirName *dirName = &disk.dataBlock[disk.dataBlock[disk.iNode[inodeid].indexedAddress].index[i]].dirName[j];
                if (dirName->inodeId == -1) {
                    continue;
                }
                if (dirName->key == key && strcmp(name, dirName->name) == 0) {
                    return dirName->inodeId;
                }
            }
        }
    }
    return -2;
}

void copy() {
    char name[40], path[40], temp[40];
    int i = 0, j = 0, inode = -1;
    cin >> name;
    cin >> path;
    a:
    for (; i < strlen(path); i++, j++) {
        if (path[i] == '/')
            goto b;
        temp[j] = path[i];
    }
    b:
    temp[j] = '\0';
    i++;
    inode = findDir(temp, inode, '0');
    j = 0;
    if (inode == -2) {
        cout << "文件夹不存在" << endl;
        return;
    }
    if (i >= strlen(path))
        goto c;
    goto a;
    c:


    INode *tempINode1;
    int inode2 = findDir(name, history.top(), '1');
    if (inode2 == -2) {
        cout << "文件不存在" << endl;
        return;
    }
    tempINode1 = &disk.iNode[inode2];
//    if (tempINode1->readonly == '1') {
//        cout << "文件只读" << endl;
//        return;
//    }

    if (strcmp(name, "..") == 0) {
        cout << "命名错误" << endl;
        return;
    }
    DirName *dirName;
    if (findDir(name, inode, '1') == -2) {
        if (inode == -1) {
            for (int i = 0; i < 4; i++) {
                if (disk.dir[i].inodeId == -1) {
                    dirName = &disk.dir[i];
                    goto d;
                }
            }
        } else {

            for (int j = 0; j < 2; ++j) {
                for (int i = 0; i < 4; i++) {
                    if (j == 1 && disk.iNode[inode].directAddress[j] == -1) {
                        disk.iNode[inode].directAddress[j] = findEmptyBlock();
                        for (int i = 0; i < 4; i++)
                            disk.dataBlock[disk.iNode[inode].directAddress[j]].dirName[i].inodeId = -1;
                    }
                    dirName = &disk.dataBlock[disk.iNode[inode].directAddress[j]].dirName[i];
                    if (dirName->inodeId == -1)
                        goto d;
                }
            }
            if (disk.iNode[inode].indexedAddress == -1) {
                disk.iNode[inode].indexedAddress = findEmptyBlock();
                for (int i = 0; i < 16; i++)
                    disk.dataBlock[disk.iNode[inode].indexedAddress].index[i] = -1;
            }

            for (int i = 0; i < 16; i++) {
                if (disk.dataBlock[disk.iNode[inode].indexedAddress].index[i] == -1) {
                    disk.dataBlock[disk.iNode[inode].indexedAddress].index[i] = findEmptyBlock();
                    for (int j = 0; j < 4; ++j) {
                        disk.dataBlock[disk.dataBlock[disk.iNode[inode].indexedAddress].index[i]].dirName[j].inodeId = -1;
                    }
                }
                for (int j = 0; j < 4; ++j) {


                    dirName = &disk.dataBlock[disk.dataBlock[disk.iNode[inode].indexedAddress].index[i]].dirName[j];
                    if (dirName->inodeId == -1) {
                        goto d;
                    }

                }
            }

        }
        cout << "当前目录已满" << endl;
        return;
        d:
        strcpy(dirName->name, name);
        dirName->key = '1';
        dirName->inodeId = findEmptyINode();

        disk.iNode[dirName->inodeId].readonly = '0';
        disk.iNode[dirName->inodeId].hidden = '0';
        // 基于当前系统的当前日期/时间
        time_t now = time(0);
        tm *ltm = localtime(&now);
        // 输出 tm 结构的各个组成部分
        disk.iNode[dirName->inodeId].hour = (unsigned char) (ltm->tm_hour);
        disk.iNode[dirName->inodeId].minute = (unsigned char) (ltm->tm_min);
        disk.iNode[dirName->inodeId].directAddress[0] = findEmptyBlock();
        disk.dataBlock[disk.iNode[dirName->inodeId].directAddress[0]].data[0] = '\0';
        disk.iNode[dirName->inodeId].directAddress[1] = -1;
        disk.iNode[dirName->inodeId].indexedAddress = -1;
    }


    char tempString[1152];
    tempString[0] = disk.dataBlock[tempINode1->directAddress[0]].data[0];
    int k = 0;
     do{
        if (k < 64) {
            tempString[k] = disk.dataBlock[tempINode1->directAddress[0]].data[k];
        } else if (k < 128) {
            tempString[k] = disk.dataBlock[tempINode1->directAddress[1]].data[k - 64];
        } else {
            tempString[k] = disk.dataBlock[disk.dataBlock[tempINode1->indexedAddress].index[(k - 128) / 64]].data[
                    (k - 128) % 64];
        }
         if(tempString[k] == '\0')
             break;
        k++;
    }while (1);
    cout<<tempString<<endl;


    int length = strlen(tempString);

    if (length > 64) {
        disk.iNode[dirName->inodeId].directAddress[1] = findEmptyBlock();
        disk.dataBlock[disk.iNode[dirName->inodeId].directAddress[1]].data[0] = '\0';
    }
    if (length > 128) {
        disk.iNode[dirName->inodeId].indexedAddress = findEmptyBlock();
        for (int i = 0; i < 16; i++)
            disk.dataBlock[disk.iNode[dirName->inodeId].indexedAddress].index[i] = -1;
    }
    for (int i = 0; i <= (length - 128) / 64 && length > 128; ++i) {
        disk.dataBlock[disk.iNode[dirName->inodeId].indexedAddress].index[i] = findEmptyBlock();
        disk.dataBlock[disk.dataBlock[disk.iNode[dirName->inodeId].indexedAddress].index[i]].data[0] = '\0';

    }


    for (int i = 0; i < 64 && i <= length; ++i) {
        disk.dataBlock[disk.iNode[dirName->inodeId].directAddress[0]].data[i] = tempString[i];
    }
    for (int i = 64; i < 128 && i <= length; ++i) {
        disk.dataBlock[disk.iNode[dirName->inodeId].directAddress[1]].data[i - 64] = tempString[i];
    }
    for (int j = 0; j < 16; ++j) {
        for (int i = 128 + j * 64; i < 192 + j * 64 && i <= length; ++i) {
            disk.dataBlock[disk.dataBlock[disk.iNode[dirName->inodeId].indexedAddress].index[j]].data[i - 128 - j *
                                                                                                                64] = tempString[i];
        }
    }
}

void attrib() {
    char att[3], name[40];
    char *attt[] = {(char *) "+r", (char *) "-r", (char *) "+h", (char *) "-h",};
    cin >> att;
    int choice = -1;

    for (int i = 0; i < 4; ++i) {
        if (strcmp(att, attt[i]) == 0) {
            choice = i;
            break;
        }
    }
    if (choice == -1) {
        cout << "输入正确命令" << endl;
        return;
    }

    cin >> name;
    int inode = findDir(name, history.top(), '1');
    if (inode != -2) {
        switch (choice) {
            case 0:
                disk.iNode[inode].readonly = '1';
                break;
            case 1:
                disk.iNode[inode].readonly = '0';
                break;
            case 2:
                disk.iNode[inode].hidden = '1';
                break;
            case 3:
                disk.iNode[inode].hidden = '0';
                break;
        }

    } else {
        cout << "文件不存在";
    }


}

int main() {
    int quit = 0;
    int choice;
    char comm[30], name[30];
    char *command[] = {(char *) "Format", (char *) "Quit", (char *) "Mkdir", (char *) "Deldir", (char *) "Cd",
                       (char *) "Dir", (char *) "Mkfile", (char *) "Delfile", (char *) "Open", (char *) "Attrib",
                       (char *) "Viewinodemap", (char *) "Viewblockmap", (char *) "Copy"};
    Format();
    while (1) {
        cout << path << "#";
        cin >> comm;
        choice = -1;

        for (int i = 0; i < 13; ++i) {
            if (strcmp(comm, command[i]) == 0) {
                choice = i;
                break;
            }
        }

        switch (choice) {
            /*格式化文件系统*/
            case 0:
                Format();
                break;
                /*退出文件系统*/
            case 1:
                quit = 1;
                break;
                /*创建子目录*/
            case 2:
                MakeFile('0');
                break;
                /*删除子目录*/
            case 3:
                DeleteFile('0');
                break;
                /*进入子目录*/
            case 4:
                scanf("%s", name);
                cd(name);
                break;
                /*显示目录内容*/
            case 5:
                listAll();
                break;
                /*创建文件*/
            case 6:
                MakeFile('1');
                break;
                /*删除文件*/
            case 7:
                DeleteFile('1');
                break;
                /*对文件进行编辑*/
            case 8:
                scanf("%s", name);
                open(name);
                break;
            case 9:
                attrib();
                break;
            case 10:
                Viewinodemap();
                break;
            case 11:
                Viewblockmap();
                break;
            case 12:
                copy();
                break;
            default:
                printf("%s command not found\n", comm);
        }

        if (quit) break;
    }
    return 0;
}