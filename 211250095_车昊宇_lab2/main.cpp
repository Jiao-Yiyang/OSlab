#include<string.h>
#include<string>
#include<vector>
#include<iostream>
//nasm -f elf -o my_print.o my_print.asm
//g++ -m32 -o main main.cpp my_print.o
using namespace std;
int getFATValue(FILE *fat12, int num);
int BytsPerSec;				//每扇区字节数
int SecPerClus;				//每簇扇区数
int RsvdSecCnt;				//Boot记录占用的扇区数
int NumFATs;				//FAT表个数
int RootEntCnt;				//根目录最大文件数
int FATSz;					//FAT扇区数
int fatBase;//FAT1的偏移字节
int fileRootBase;
int dataBase;
int BytsPerClus;

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
extern "C"
{
void my_printRed(char *a, int len);
void my_print(char* a, int len);
}
void Print(string a, int type){ //汇编实现的输出函数
    char *out = new char[a.length()+1]; 
    strcpy(out , a.c_str());
    if(type){   
        my_printRed(out,a.length());
    }else{
        my_print(out,a.length());
    }
}

vector<string> split(string str,string tosplit){
    vector<string> res;
    char* a = new char[str.length()+1];
    strcpy(a, str.c_str());
    char* b = new char[tosplit.length()+1];
    strcpy(b,tosplit.c_str());
    char *p = strtok(a, b);
    while (p) {
        string s = p; 
        res.push_back(s);
        p = strtok(NULL, b);
    }
    return res;
}

#pragma pack(1) /*指定按1字节对齐*/
class BPB
{
    public:
    ushort BPB_BytsPerSec; //每扇区字节数
    uchar BPB_SecPerClus;  //每簇扇区数
    ushort BPB_RsvdSecCnt; //Boot记录占用的扇区数
    uchar BPB_NumFATs;		//FAT表个数
    ushort BPB_RootEntCnt; //根目录最大文件数
    ushort BPB_TotSec16;//总扇区数
    uchar BPB_Media;//介质描述符
    ushort BPB_FATSz16; //FAT扇区数
    ushort BPB_SecPerTrk;//每磁道扇区数
    ushort BPB_NumHeads; //磁头数/面数
    uint BPB_HiddSec;//隐藏扇区数
    uint BPB_TotSec32; //如果BPB_FATSz16为0，该值为FAT扇区数
    BPB(FILE *fat12) {
        fseek(fat12, 11, SEEK_SET);   //BPB从偏移11个字节处开始
        fread(this, 1, 25, fat12); //BPB长度为25字节
    };
    void bpb_init() { //初始化各个全局变量
    BytsPerSec = this->BPB_BytsPerSec;
    SecPerClus = this->BPB_SecPerClus;
    RsvdSecCnt = this->BPB_RsvdSecCnt;
    NumFATs = this->BPB_NumFATs;
    RootEntCnt = this->BPB_RootEntCnt;
    if (this->BPB_FATSz16 != 0){
        FATSz = this->BPB_FATSz16;
    }
    else{
        FATSz = this->BPB_TotSec32;
    }
    // ↓计算每簇的字节数，每个区域的初始位置的偏移量
    // 每簇字节数=每簇扇区数*每扇区字节数
    BytsPerClus = SecPerClus * BytsPerSec;
    // fatBase=Boot记录占用的扇区数*每扇区字节数
    fatBase = RsvdSecCnt * BytsPerSec; 
    // fileRootBase=（Boot记录占用的扇区数+FAT表个数* FAT扇区数）*每扇区字节数
    fileRootBase = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec; //根目录首字节的偏移数=boot+fat1&2的总字节数
    // dataBase=（Boot记录占用的扇区数+FAT表个数* FAT扇区数+（根目录最大文件数* 32+每扇区字节数-1）/每扇区字节数）*每扇区字节数
    dataBase = BytsPerSec * (RsvdSecCnt + FATSz * NumFATs + (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec);
    }
//BPB结束，长度25字节
};
class Node{
public:
    string path;
    string name;
    vector<Node*> next;
    Node* father;
    uint FileSize;
    bool isFile = false;
    bool isVal = true;
    int dir_count = 0;
    int file_count = 0;
    char *content = new char[10000];
    Node(){};
    Node(string name,string path):name(name),path(path){};
    Node(string name,string path,Node* father):father(father),name(name),path(path){};
    Node(string name,string path,uint size, bool isFile,Node* father):father(father),isFile(isFile),FileSize(size),name(name),path(path){};
};
void readFilecontent(FILE *fat12, int startClus, Node *root){
    // dataBase=（Boot记录占用的扇区数+FAT表个数* FAT扇区数+（根目录最大文件数* 32+每扇区字节数-1）/每扇区字节数）*每扇区字节数
    int base = BytsPerSec * (RsvdSecCnt + FATSz * NumFATs + (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec);
    int currentClus = startClus;
    int value = 0;
    if (startClus == 0) return;
    int temp =0;
    while (value < 0xFF8) {
        value = getFATValue(fat12, currentClus);
        if (value == 0xFF7) {
            break;
        }
        int size = SecPerClus * BytsPerSec;
        char *content = new char[size];
        int startByte = base + (currentClus - 2)*SecPerClus*BytsPerSec;    // 用FAT号计算在数据区中的位置。
        fseek(fat12, startByte, SEEK_SET);
        fread(content, 1, size, fat12);
        for (int i = 0; i < size; ++i) {
            root->content[temp] = content[i];
            temp++;
        }
        currentClus = value;
    }
}
void readChildren(FILE *fat12, int startClus, Node *root);
class Entry{
    public:
    char DIR_name[11];//文件名
    uchar DIR_attribute;//文件数性
    char reserved[10];
    ushort DIR_WrtTime;
    ushort DIR_WrtDate;
    ushort DIR_FstClus; //开始簇号
    uint DIR_FileSize;
    Entry() {};
    Entry(FILE *fat12, Node *root){
        int temp = fileRootBase;
        char* filename = new char[12];
        for(int i = 0; i < RootEntCnt; i++){
            fseek(fat12,temp,SEEK_SET);
            fread(this,1,32,fat12);
            temp += 32;
            if((this->DIR_name[0]=='\0' || isInvalidName())){
                continue;
            }
        
            if (this->isFile()) {
                getFileName(filename);
                Node *child = new Node(filename, root->path, this->DIR_FileSize, true,root);
                root->next.push_back(child);
                root->file_count++;
                readFilecontent(fat12, DIR_FstClus, child);
            } else {
                getDirName(filename);
                Node *child = new Node(filename,root->path +filename + "/",root);
                root->next.push_back(child);
                root->dir_count++;
                readChildren(fat12, DIR_FstClus, child);
        }
        }
    };

    bool isFile() {
    // 用DIR_Attr&0x10判断，结果为0是文件，否则为文件夹
    return (this->DIR_attribute & 0x10) == 0;
    }
    
    bool isInvalidName() {
    for (int j = 0; j < 11; j++) {
        if (! (((DIR_name[j] >= 'a') && (DIR_name[j] <= 'z'))
            ||((DIR_name[j] >= 'A') && (DIR_name[j] <= 'Z'))
            ||((DIR_name[j] >= '0') && (DIR_name[j] <= '9'))
            ||((DIR_name[j] == ' ')))){
            return true;
        }
    }
    return false;
    }

    void getFileName(char* name) {
    int temp = 0;
    for (int j = 0; j < 11; j += 1) {
        if (DIR_name[j] != ' ') {
            name[temp] = DIR_name[j];
            temp += 1;
        } else {
            name[temp] = '.';
            while (DIR_name[j] == ' ') {
                j += 1;
            }
            j -= 1;
            temp += 1;
        }
    }
    name[temp] = '\0';
    }
    void getDirName(char *name) {
    int temp = -1;
    for (int k = 0; k < 11; ++k) {
        if (this->DIR_name[k] != ' ') {
            temp += 1;
            name[temp] = this->DIR_name[k];
        } else {
            temp += 1;
            name[temp] = '\0';
            break;
        }
    }
    }
};

int getFATValue(FILE *fat12, int num) {
    int base = RsvdSecCnt * BytsPerSec;
    // 从fatBase + num * 3 / 2读取2个字节（16位）。
    // 结合存储的小尾顺序和FAT项结构可以得到。num为偶去掉高4位,num为奇去掉低4位。
    int pos = base + num * 3 / 2;
    int type = num % 2;

    ushort bytes;
    ushort *bytesPtr = &bytes;
    fseek(fat12, pos, SEEK_SET);
    fread(bytesPtr, 1, 2, fat12);

    if (type == 0) {
        bytes = bytes << 4;
    }
    return bytes >> 4;
}
void readChildren(FILE *fat12, int startClus, Node *root) {
    int base = BytsPerSec * (RsvdSecCnt + FATSz * NumFATs + (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec);
    int currentClus = startClus;
    int value = 0;
    // 0xFF8 End; 0xFF7 Bad Cluster
    while (value < 0xFF8) {
        value = getFATValue(fat12, currentClus);
        if (value == 0xFF7) {
            Print("Error!", 0);
            break;
        }
        int startByte = base + (currentClus - 2) * SecPerClus * BytsPerSec;
        int size = SecPerClus * BytsPerSec;
        int temp = 0;
        while (temp < size) {
            Entry *entry = new Entry();
            fseek(fat12, startByte + temp, SEEK_SET);
            fread(entry, 1, 32, fat12);
            temp += 32;
            if ((entry->DIR_name[0]=='\0' || entry->isInvalidName())) {
                continue;
            }
            char filename[12];
            if ((entry->isFile())) {
                entry->getFileName(filename);
                Node *child = new Node(filename, root->path, entry->DIR_FileSize, true,root);
                root->next.push_back(child);
                root->file_count++;
                readFilecontent(fat12, entry->DIR_FstClus, child);
            } else {
                entry->getDirName(filename);
                Node *child = new Node(filename,root->path +filename + "/",root);
                root->next.push_back(child);
                root->dir_count++;
                readChildren(fat12, entry->DIR_FstClus, child);
            }
        }
    }
}
void print_ls(Node* root){
    Print(root->path+":\n",0);
    Print(".  ..  ",1);
    for(int i = 0; i < root->next.size();i++){
        if(root->next[i]->isFile){
            Print(root->next[i]->name+"  ",0);
        }else{
            Print(root->next[i]->name+"  ",1);
        }
    }
    Print("\n",0);
    return;
}

void print_ls_l(Node* root){
    Print(root->path+" "+to_string(root->dir_count)+" "+to_string(root->file_count)+":\n",0);
    Print(".\n..\n",1);
    for(int i = 0; i < root->next.size();i++){
        if(root->next[i]->isFile){
            Print(root->next[i]->name+" "+to_string(root->next[i]->FileSize)+"\n",0);
        }else{
            Print(root->next[i]->name+" "+to_string(root->next[i]->dir_count)+" "+to_string(root->next[i]->file_count)+"\n",1);
        }
    }
    Print("\n",0);
    return;
}
void dfs_node(Node* root,int type){
    if(type){
        print_ls_l(root);
    }else{
        print_ls(root);
    }
    for(int i = 0; i < root->next.size();i++){
        if(!root->next[i]->isFile){
            dfs_node(root->next[i],type);
        }
    }
    return;
}
Node* find_dir(string path,Node* root){
    vector<string> paths;
    paths = split(path,"/");
    Node* res = root;
    for(int i =0;i < paths.size();i++){
        if(paths[i]=="."){
            continue;
        }else if(paths[i]==".."){
            res = res->father;
        }else{
            bool mark = false;
            for(int j = 0 ; j < res->next.size();j++){
                if(!res->next[j]->isFile&&res->next[j]->name == paths[i]){
                    res = res->next[j];
                    mark = true;
                    break;
                }
            }
            if(!mark){
                return nullptr;
            }
        }
    }
    return res;
}
Node* find_file(string path,Node* root){
    vector<string> paths;
    paths = split(path,"/");
    Node* res = root;
    for(int i =0;i < paths.size()-1;i++){
        if(paths[i]=="."){
            continue;
        }else if(paths[i]==".."){
            res = res->father;
        }else{
            bool mark = false;
            for(int j = 0 ; j < res->next.size();j++){
                if(!res->next[j]->isFile&&res->next[j]->name == paths[i]){
                    res = res->next[j];
                    mark = true;
                    break;
                }
            }
            if(!mark){
                return nullptr;
            }
        }
    }
    return res;
}
void cat(Node* root,vector<string> cmd){
    Node* t = nullptr;
    if(cmd.size()!=2){
        if(cmd.size()==1){
            Print("Error: No such file or directory!\n",0);
            return;
        }else{
            Print("Error: More than one address!\n",0);
            return;
        }
    }
    vector<string> paths;
    paths = split(cmd[1],"/");
    if(cmd[1][0]=='/'&&paths.size()==1){
                Print("Command error: "+ cmd[1]+" is not a correct path!\n",0);
                return;
            }
    t = find_file(cmd[1],root);
    if(t == nullptr){
        Print("Command error: "+ cmd[1]+" is not a correct path!\n",0);
        return;
    }
    
    bool mark = false;
    for(int i =0;i< t->next.size();i++){
        if(t->next[i]->isFile&&t->next[i]->name == paths[paths.size()-1]){
            t = t->next[i];
            mark = true;
            break;
        }
    }
    if(!mark){
        Print("Command Error: "+ paths[paths.size()-1]+" no such file!\n",0);
        return;
    }
    string out = t->content;
    Print(out,0);
    return;
}
void ls(Node* root,vector<string> cmd){
    if(cmd.size()==1){
        dfs_node(root,0);
        return;
    }
    Node* t= root;
    int givenPath =-1;
    bool given_l = false;
    for(int i = 1;i < cmd.size();i++){
        if(cmd[i][0]=='-'){
            for(int j = 1 ; j < cmd[i].length();j++){
                if(cmd[i][j]!='l'){
                    Print("Command error: "+ cmd[i]+" is an invalid option!\n",0);
                    return;
                }
            }
            given_l = true;
        }else{
            if(givenPath>0){
                Print("Command error: more than one path!\n",0);
                return;
            }
            t = find_dir(cmd[i],root);
            if(t == nullptr){
                Print("Command error: "+ cmd[i]+" is not a correct path!\n",0);
                return;
            }
            givenPath = i;
        }
    }
    if(given_l){
        dfs_node(t,1);
        return;
    }else{
        dfs_node(t,0);
        return;
    }
}

int main(){
    FILE *fat12;
    fat12 = fopen("/mnt/hgfs/OS/211250095_车昊宇_lab2/a.img", "rb"); //打开FAT12的映像文件 
    BPB *bpb = new BPB(fat12);
    bpb->bpb_init();
    Node* root = new Node("","/");
    root->father = root;
    Entry *entry = new Entry(fat12, root);
    while(true){
        Print("> ",0);
        string a;
        getline(cin, a);
        vector<string> cmd = split(a," ");
        if(cmd.empty()){
            Print("Command not found!\n",0);
            continue;
        }
        if(cmd[0] == "exit"){
            fclose(fat12);
            break;
        }else if(cmd[0] == "cat"){
            cat(root,cmd);
        }else if(cmd[0] == "ls"){
            ls(root,cmd);
        }else{
            Print("Command not found!\n",0);
            continue;
        }
    }
    return 0;
}