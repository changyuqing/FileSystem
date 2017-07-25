#include <iostream>
#include <iomanip>
#include <cstring>
#include <bitset>
#include <stack>
#include <ctime>

using namespace std;
using std::bitset;

struct DirName {
    char name[11];      //�ļ�����Ŀ¼��
    unsigned char key;  //Ŀ¼��־
    int inodeId;        //Inode���

    DirName() {         //��ʼ���ṹ��
        strcpy(name, "");
        key = '2';
        inodeId = -1;
    }
};

struct INode {              //Inode��
    unsigned char readonly; //ֻ��
    unsigned char hidden;   //����
    unsigned char hour;     //Сʱ
    unsigned char minute;   //����
    int directAddress[2];   //ֱ���̿��ַ
    int indexedAddress;     //һ��������
};

union DataBlock {       //���ݿ�
    DirName dirName[4]; //Ŀ¼�ļ����ݿ�
    int index[16];      //������
    char data[64];      //�ı��ļ����ݿ�
    DataBlock() {
    }
};

struct Disk {
    DirName dir[4];             //��Ŀ¼
    bitset<516> inodeBitMap;    //I-nodeλͼ
    bitset<1024> blockBitMap;   //���ݿ�λͼ
    INode iNode[128];           //I-node
    DataBlock dataBlock[1024];  //���ݿ�

    Disk() {
    }
};


char path[40] = "root"; //��ǰ·��
Disk disk;              //�������
int dirNum;             //��Ӧ��ŵ�Ŀ¼����
INode *currInode;       //��ǰĿ¼��inode�ṹ
stack<int> history;     //·��ջ

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
        cout << "��������" << endl;
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
        cout << "��ǰĿ¼����" << endl;
        return;
        a:
        strcpy(dirName->name, name);
        dirName->key = key;
        dirName->inodeId = findEmptyINode();

        disk.iNode[dirName->inodeId].readonly = '0';
        disk.iNode[dirName->inodeId].hidden = '0';
        // ���ڵ�ǰϵͳ�ĵ�ǰ����/ʱ��
        time_t now = time(0);
        tm *ltm = localtime(&now);
        // ��� tm �ṹ�ĸ�����ɲ���
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
    } else cout << "�ļ����ļ����Ѵ���" << endl;
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
                    cout << "�ļ�������" << endl;
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
            cout << "�ļ�������" << endl;
            return;
        }
        for (int i = 0; i < 16; i++) {
            if (disk.dataBlock[currInode->indexedAddress].index[i] == -1) {
                cout << "�ļ�������" << endl;
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
    cout << "�ļ�������" << endl;
    return;
    a:
    if (disk.iNode[tempDir->inodeId].readonly == '1') {
        cout << "�ļ�ֻ��" << endl;
        return;
    }
    if (tempDir->key == '0') {
        if (disk.dataBlock[disk.iNode[tempDir->inodeId].directAddress[0]].dirName[0].inodeId != -1) {

            cout << "�ļ��в�Ϊ��" << endl;
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
    if (strcmp(name, "..") == 0) {//�����ϲ�Ŀ¼�������һ��'/'�������ȥ��
        pos = strlen(path) - 1;
        for (; pos >= 0; --pos) {
            if (path[pos] == '/') {
                path[pos] = '\0';
                break;
            }
        }
    } else {//������·��ĩβ�����Ŀ¼
        strcat(path, "/");
        strcat(path, name);
    }
    return;
}

void cd(char name[]) {
    if (strcmp(name, "..") == 0)
        if (history.top() == -1) {
            cout << "�޷������ϲ�" << endl;
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
        cout << "�ļ��в�����" << endl;
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
        cout << "�ļ�������" << endl;
        return;
    }
    tempINode = &disk.iNode[inodenum];
    if (tempINode->readonly == '1') {
        cout << "�ļ�ֻ��" << endl;
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
    cout << "��ǰ�ı�����" << endl;
    cout << tempString << endl;
    cout << "�������µ�����" << endl;
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
        cout << "�ļ��в�����" << endl;
        return;
    }
    if (i >= strlen(path))
        goto c;
    goto a;
    c:


    INode *tempINode1;
    int inode2 = findDir(name, history.top(), '1');
    if (inode2 == -2) {
        cout << "�ļ�������" << endl;
        return;
    }
    tempINode1 = &disk.iNode[inode2];
//    if (tempINode1->readonly == '1') {
//        cout << "�ļ�ֻ��" << endl;
//        return;
//    }

    if (strcmp(name, "..") == 0) {
        cout << "��������" << endl;
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
        cout << "��ǰĿ¼����" << endl;
        return;
        d:
        strcpy(dirName->name, name);
        dirName->key = '1';
        dirName->inodeId = findEmptyINode();

        disk.iNode[dirName->inodeId].readonly = '0';
        disk.iNode[dirName->inodeId].hidden = '0';
        // ���ڵ�ǰϵͳ�ĵ�ǰ����/ʱ��
        time_t now = time(0);
        tm *ltm = localtime(&now);
        // ��� tm �ṹ�ĸ�����ɲ���
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
        cout << "������ȷ����" << endl;
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
        cout << "�ļ�������";
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
            /*��ʽ���ļ�ϵͳ*/
            case 0:
                Format();
                break;
                /*�˳��ļ�ϵͳ*/
            case 1:
                quit = 1;
                break;
                /*������Ŀ¼*/
            case 2:
                MakeFile('0');
                break;
                /*ɾ����Ŀ¼*/
            case 3:
                DeleteFile('0');
                break;
                /*������Ŀ¼*/
            case 4:
                scanf("%s", name);
                cd(name);
                break;
                /*��ʾĿ¼����*/
            case 5:
                listAll();
                break;
                /*�����ļ�*/
            case 6:
                MakeFile('1');
                break;
                /*ɾ���ļ�*/
            case 7:
                DeleteFile('1');
                break;
                /*���ļ����б༭*/
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