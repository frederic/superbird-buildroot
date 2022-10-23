/*
 * Command for pack imgs.
 *
 * Copyright (C) 2012 Amlogic.
 * Elvis Yu <elvis.yu@amlogic.com>
 */
#include "res_pack_i.h"

#define COMPILE_TYPE_CHK(expr, t)       typedef char t[(expr) ? 1 : -1]

COMPILE_TYPE_CHK(AML_RES_IMG_HEAD_SZ == sizeof(AmlResImgHead_t), a);//assert the image header size 64
COMPILE_TYPE_CHK(AML_RES_ITEM_HEAD_SZ == sizeof(AmlResItemHead_t), b);//assert the item head size 64

#define IMG_HEAD_SZ     sizeof(AmlResImgHead_t)
#define ITEM_HEAD_SZ    sizeof(AmlResItemHead_t)
//#define ITEM_READ_BUF_SZ    (1U<<20)//1M
#define ITEM_READ_BUF_SZ    (64U<<10)//64K to test


typedef int (*pFunc_getFile)(const char** const , __hdle *, char* );

static size_t get_filesize(const char *fpath)
{
	struct stat buf;
	if(stat(fpath, &buf) < 0)
	{
		fprintf (stderr, "Can't stat %s : %s\n", fpath, strerror(errno));
		exit (EXIT_FAILURE);
	}
	return buf.st_size;
}


static  const char* get_filename(const char *fpath)
{
#if 1
	int i;
	const char *filename = fpath;
	for(i=strlen(fpath)-1; i>=0; i--)
	{
		if('/' == fpath[i] || '\\' == fpath[i])
		{
			i++;
			filename = fpath + i;
			break;
		}
	}

	return filename;
#else
	return (strrchr(fpath, '/')+1);
#endif
}

// added to filter out .bmp files
static char* get_last_itemname(const char* itemName)
{
    char buf[256]={0};
    char* last_itemname = (char*)malloc(256);

    strcpy(last_itemname, itemName);
    const char* bmp_tag = ".bmp";
    const char* suffix_of_file;
    suffix_of_file = strstr(itemName, bmp_tag);
    int len = 0;

    if (suffix_of_file && strcmp(suffix_of_file, bmp_tag) == 0)
    {
        len = suffix_of_file-itemName;
        strncpy(buf, itemName, len);
        buf[len] = '\0';
        strcpy(last_itemname, buf);
    }
    return last_itemname;
}

int get_file_path_from_argv(const char** const argv, __hdle *hDir, char* fileName)
{
    long index = (long)hDir;
    const char* fileSrc = argv[index];

    strcpy(fileName, fileSrc);
    return 0;
}

#if defined(WIN32)
static __hdle _find_first_file(const char* const dirPath, WIN32_FIND_DATA& findFileData)
{
    HANDLE hdle = INVALID_HANDLE_VALUE;
    TCHAR szDir[MAX_PATH];
    size_t length_of_arg;
    // Check that the input path plus 3 is not longer than MAX_PATH.
    // Three characters are for the "\*" plus NULL appended below.
    StringCchLength(dirPath, MAX_PATH, &length_of_arg);

    if (length_of_arg > (MAX_PATH - 3))
    {
        _tprintf(TEXT("\nDirectory path is too long.\n"));
        return NULL;
    }

    _tprintf(TEXT("\nTarget directory is %s\n\n"), dirPath);

    // Prepare string for use with FindFile functions.  First, copy the
    // string to a buffer, then append '\*' to the directory name.

    StringCchCopy(szDir, MAX_PATH, dirPath);
    StringCchCat(szDir, MAX_PATH, TEXT("\\*"));

    hdle = FindFirstFile(szDir, &findFileData);
    if(INVALID_HANDLE_VALUE == hdle){
        errorP("Fail to open dir\n");
        return NULL;
    }

    return hdle;
}

int traverse_dir(const char** const dirPath, __hdle *hDir, char* filePath)
{
  WIN32_FIND_DATA findFileData;
  HANDLE hdle = INVALID_HANDLE_VALUE;
  int ret = 0;

  if (!*hDir)//not open yet!
  {
      hdle = _find_first_file(*dirPath, findFileData);
      if(INVALID_HANDLE_VALUE == hdle){
          errorP("Fail to find first file in dir(%s)\n", *dirPath);
          return __LINE__;
      }
      *hDir = hdle;
      if(strcmp(".", findFileData.cFileName) && strcmp("..", findFileData.cFileName))goto _ok;
  }

  hdle = *hDir;
  do 
  {
    ret = !FindNextFile(hdle, &findFileData);
    if(ret){
      debugP("Find end.\n");
      FindClose(hdle);
      return __LINE__;
    }
  } while(!strcmp(".", findFileData.cFileName) || !strcmp("..", findFileData.cFileName) );
  
_ok: 
  //debugP("file %s\n", findFileData.cFileName);
  if(MAX_PATH < strlen(findFileData.cFileName)){
    errorP("Buffer samll\n");
    FindClose(hdle);
    return __LINE__;
  }
  sprintf(filePath, "%s\\%s", *dirPath, findFileData.cFileName);
  return ret;
}

int get_dir_filenums(const char * const dirPath)
{
    WIN32_FIND_DATA findFileData;
    HANDLE hdle = INVALID_HANDLE_VALUE;
    int fileNum = 0;
    int ret = 0;
    TCHAR szDir[MAX_PATH];
    size_t length_of_arg;

    hdle = _find_first_file(dirPath, findFileData);
    if(INVALID_HANDLE_VALUE == hdle){
        errorP("Fail to open dir %s\n", szDir);
        return __LINE__;
    }

    do {
        if(strcmp(findFileData.cFileName, ".") && strcmp(findFileData.cFileName, ".."))
        {
          ++fileNum;
        }
    } while (FindNextFile(hdle, &findFileData));

_ok: 
    if(INVALID_HANDLE_VALUE != hdle)FindClose(hdle);
    return fileNum;
}

#else//Follwing is for Linux platform
int get_dir_filenums(const char *dir_path)
{
	int count = 0;
	DIR *d;
	struct dirent *de;
	d = opendir(dir_path);
    if(d == 0) {
        fprintf(stderr, "opendir failed, %s\n", strerror(errno));
        return -1;
    }
	while((de = readdir(d)) != 0){
	        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
	        if(de->d_name[0] == '.') continue;
		count++;
	}
    closedir(d);
	return count;
}

int traverse_dir(const char** const dirPath, __hdle *hdle, char* filePath)
{
    DIR* hDir = (DIR*)*hdle;
    const char* fileName = NULL;

    if(!hDir)
    {
        hDir = opendir(*dirPath);
        if(!hDir){
            errorP("Fail to open dir(%s), strerror(%s)\n", *dirPath, strerror(errno));
            return __LINE__;
        }
        *hdle = hDir;
    }

    do{
        struct dirent* dirEntry = NULL;

        dirEntry = readdir(hDir);
        if(!dirEntry){
            debugP("travese end.\n");
            closedir(hDir);
            return __LINE__;
        }
        fileName = dirEntry->d_name;
    }while(!strcmp(".", fileName) || !strcmp("..", fileName));

    sprintf(filePath, "%s/%s", *dirPath, fileName);
    return 0;
}
#endif//#if defined(WIN32)

//@imgVersion: if success, then 0--> oldest version; 1-->img header + n * (item header + item body); 2 --> img header + n * item header + n * item body
static int _img_unpack_check_img_header(FILE* fdResImg, const char* const path_src, 
                int needCheckCrc, AmlResImgHead_t* pImgHead, int* imgVersion)
{
        int ret = 0;
        const unsigned ImgFileSz = get_filesize(path_src);

        if(ImgFileSz <= IMG_HEAD_SZ){
                errorP("file size 0x%x too small\n", ImgFileSz);
                return __LINE__;
        }

        unsigned thisReadSz = ITEM_READ_BUF_SZ > ImgFileSz ? ImgFileSz : ITEM_READ_BUF_SZ;
        unsigned actualReadSz = 0;

        actualReadSz = fread(pImgHead, 1, thisReadSz, fdResImg);
        if(actualReadSz != thisReadSz){
                errorP("Want to read 0x%x, but only read 0x%x\n", thisReadSz, actualReadSz);
                return __LINE__;
        }

        if(!strncmp(AML_RES_IMG_V1_MAGIC, (char*)pImgHead->magic, AML_RES_IMG_V1_MAGIC_LEN))
        {//new version magic matched
                if(ImgFileSz < pImgHead->imgSz){
                        errorP("error, image size in head 0x%x != fileSz 0x%x\n", pImgHead->imgSz, ImgFileSz);
                        return __LINE__;
                }
                if(AML_RES_IMG_VERSION_V2 < pImgHead->version){
                        errorP("Error, version 0x%x not supported\n", pImgHead->version);
                        return __LINE__;
                }
                *imgVersion = pImgHead->version;

                if(needCheckCrc)
                {
                        ret = check_img_crc(fdResImg, 4, pImgHead->crc);
                        if(ret){
                                errorP("Error when check crc\n");
                                return __LINE__;
                        }
                }
                unsigned ImgHeadSz = IMG_HEAD_SZ;
                if(AML_RES_IMG_VERSION_V2 == pImgHead->version) ImgHeadSz += pImgHead->imgItemNum * ITEM_HEAD_SZ;
                fseek(fdResImg, ImgHeadSz, SEEK_SET);//seek escape the image header to prepare for reading item header
        }
        else
        {
                AmlResItemHead_t* pItemHeadInfo = (AmlResItemHead_t*)pImgHead;
                debugP("magic error, try old version image.\n");
                if(IH_MAGIC != pItemHeadInfo->magic){
                        errorP("magic err, old version image header not item header!\n");
                        return __LINE__;
                }
                *imgVersion = 0;
                fseek(fdResImg, 0, SEEK_SET);//seek escape the image header to prepare for reading item header
        }

        debugP("res-img ver is 0x%x\n", *imgVersion);
        return 0;
}

//Get the item header info
//If the @imgVersion is V2, the item header already stored in @pImgHead, if @imgVersion is V2 or old, also read it into 64K size pImgHead
static const AmlResItemHead_t* _img_unpack_get_item_header(const int imgVersion, const AmlResImgHead_t* pImgHead, int itemIndex, FILE* fdResImg)
{
        const AmlResItemHead_t* pItemNext = NULL;
        AmlResItemHead_t*       tmpItem   = NULL;

        tmpItem  = (AmlResItemHead_t*)(pImgHead + 1);
        tmpItem += itemIndex;

        switch(imgVersion) {
                case AML_RES_IMG_VERSION_V2:
                        pItemNext = tmpItem;
                        break;

                case AML_RES_IMG_VERSION_V1:
                case 0:
                        {
                                const unsigned readSz = fread(tmpItem, 1, ITEM_HEAD_SZ, fdResImg);
                                if(ITEM_HEAD_SZ != readSz){
                                        errorP("Fail to read item head\n");
                                        return NULL;
                                }
                                pItemNext = tmpItem;
                        }
                        break;
                default:
                        break;
        }

        if(IH_MAGIC != pItemNext->magic){
                errorP("Item magic error.\n");
                return NULL;
        }

        return pItemNext;
}

#ifdef BUILD_DLL
DLL_API
#endif// #ifdef BUILD_DLL
int res_img_unpack(const char* const path_src, const char* const unPackDirPath, int needCheckCrc)
{
        char* itemReadBuf = NULL;
        FILE* fdResImg = NULL;
        int ret = 0;
        AmlResImgHead_t* pImgHead = NULL;
        int imgVersion = -1;

        fdResImg = fopen(path_src, "rb");
        if(!fdResImg){
                errorP("Fail to open res image at path %s\n", path_src);
                return __LINE__;
        }

        itemReadBuf = new char[ITEM_READ_BUF_SZ * 2];
        if(!itemReadBuf){
                errorP("Fail to new buffer at size 0x%x\n", ITEM_READ_BUF_SZ*2);
                fclose(fdResImg), fdResImg = NULL; return __LINE__;
        }
        pImgHead = (AmlResImgHead_t*)(itemReadBuf + ITEM_READ_BUF_SZ);

        ret = _img_unpack_check_img_header(fdResImg, path_src, needCheckCrc, pImgHead, &imgVersion);
        if(ret){
                errorP("Fail at check image header\n");
                delete[] itemReadBuf, itemReadBuf = NULL;
                fclose(fdResImg), fdResImg = NULL;
                return __LINE__;
        }

        //for each loop: 
        //    1, read item body and save as file; 
        //    2, get next item head
        const unsigned   itemAlignSz  = AML_RES_IMG_ITEM_ALIGN_SZ;
        const unsigned   itemAlignMod = itemAlignSz - 1;
        const unsigned   itemSzAlignMask = ~itemAlignMod;
        unsigned totalReadItemNum = 0;
        const AmlResItemHead_t* pItemHead = NULL;

        do{
                char itemFullPath[MAX_PATH*2];//TODO:512 is enough ??
                FILE* fp_item = NULL;

                pItemHead =  _img_unpack_get_item_header(imgVersion, pImgHead, totalReadItemNum++, fdResImg);
                if(!pItemHead){
                        errorP("Fail to get item header at index %d\n", totalReadItemNum - 1);
                        goto _exit;
                }

                sprintf(itemFullPath, "%s/%s", unPackDirPath, pItemHead->name);
                debugP("item %s\n", itemFullPath);

                fp_item = fopen(itemFullPath, "wb");
                if(!fp_item){
                        errorP("Fail to create file %s, strerror(%s)\n", itemFullPath, strerror(errno));
                        ret = __LINE__; goto _exit;
                }

                const unsigned thisItemBodySz = pItemHead->size; 
                const unsigned thisItemBodyOccupySz =  (thisItemBodySz & itemSzAlignMask) + itemAlignSz;
                const unsigned stuffLen       = thisItemBodyOccupySz - thisItemBodySz;
                
                for(unsigned itemTotalReadLen = 0; itemTotalReadLen < thisItemBodyOccupySz; )
                {
                        const unsigned leftLen = thisItemBodyOccupySz - itemTotalReadLen;
                        const unsigned thisReadSz = MIN(leftLen, ITEM_READ_BUF_SZ);

                        unsigned actualReadSz = fread(itemReadBuf, 1, thisReadSz, fdResImg);
                        if(thisReadSz != actualReadSz){
                                errorP("thisReadSz 0x%x != actualReadSz 0x%x\n", thisReadSz, actualReadSz);
                                ret = __LINE__;goto _exit;
                        }
                        itemTotalReadLen += thisReadSz;

                        const unsigned thisWriteSz = itemTotalReadLen < thisItemBodySz ? thisReadSz : (thisReadSz - stuffLen);
                        actualReadSz = fwrite(itemReadBuf, 1, thisWriteSz, fp_item);
                        if(thisWriteSz != actualReadSz){
                                errorP("want write 0x%x, but 0x%x\n", thisWriteSz, actualReadSz);
                                ret = __LINE__;goto _exit;
                        }

                }
                fclose(fp_item), fp_item = NULL;

        }while(pItemHead->next);


_exit:
        if(itemReadBuf)delete[] itemReadBuf, itemReadBuf = NULL;
        if(fdResImg)fclose(fdResImg), fdResImg = NULL;
        return ret;
}

/*
 * 1,
 */
static int _img_pack(const char** const path_src, const char* const packedImg, 
        pFunc_getFile getFile, const int totalFileNum)
{
        FILE *fd_src = NULL;
        FILE *fd_dest = NULL;
        unsigned int pos = 0;
        char file_path[MAX_PATH];
        const char *filename = NULL;
        unsigned imageSz = 0;
        const unsigned BufSz = ITEM_READ_BUF_SZ;
        char* itemBuf = NULL;
        unsigned thisWriteLen = 0;
        unsigned actualWriteLen = 0;
        int ret = 0;
        __hdle hDir = NULL;
        const unsigned   itemAlignSz  = AML_RES_IMG_ITEM_ALIGN_SZ;
        const unsigned   itemAlignMod = itemAlignSz - 1;
        const unsigned   itemSzAlignMask = ~itemAlignMod;
        const unsigned   totalItemNum = totalFileNum ? totalFileNum : get_dir_filenums(*path_src);
        const unsigned   HeadLen    = IMG_HEAD_SZ + ITEM_HEAD_SZ * totalItemNum;

        if(HeadLen > BufSz){
                errorP("head size 0x%x > max(0x%x)\n", HeadLen, BufSz); return __LINE__;
        }
        fd_dest = fopen(packedImg, "wb+");
        if(NULL == fd_dest)	{
                fprintf(stderr,"open %s failed: %s\n", packedImg, strerror(errno));
                return __LINE__;
        }

        itemBuf = new char[BufSz * 2];
        if(!itemBuf){
                errorP("Exception: fail to alloc buuffer\n");
                fclose(fd_dest); return __LINE__;
        }
        memset(itemBuf, 0, BufSz*2);
        AmlResImgHead_t* const aAmlResImgHead = (AmlResImgHead_t*)(itemBuf + BufSz);

        actualWriteLen = fwrite(aAmlResImgHead, 1, HeadLen, fd_dest);
        if(actualWriteLen != HeadLen){
                errorP("fail to write head, want 0x%x, but 0x%x\n", HeadLen, actualWriteLen);
                fclose(fd_dest); delete[] itemBuf;
                return __LINE__;
        }
        imageSz += HeadLen; //Increase imageSz after pack each item
        AmlResItemHead_t*       pItemHeadInfo           = (AmlResItemHead_t*)(aAmlResImgHead + 1);
        AmlResItemHead_t* const pFirstItemHeadInfo      = pItemHeadInfo;

        debugP("item num %d\n", totalItemNum);
        //for each loop: first create item header and pack it, second pack the item data
        //Fill the item head, 1) magic, 2)data offset, 3)next head start offset
        for(unsigned itemIndex = 0; itemIndex < totalItemNum; ++itemIndex)
        {
                char filePath[MAX_PATH*2];

                if(totalFileNum)//File list mode
                {
                        if ((*getFile)(path_src, (__hdle*)(int64_t)itemIndex, filePath)) {
                                break;
                        }
                }
                else
                {//file directory mode
                        if((*getFile)(path_src, &hDir, filePath)) {
                                break;
                        }
                }
                const size_t itemSz = get_filesize(filePath);
                const char*  itemName = get_filename(filePath);
                const unsigned itemBodyOccupySz =  (itemSz & itemSzAlignMask) + itemAlignSz;
                const unsigned itemStuffSz      = itemBodyOccupySz - itemSz;

                if(IH_NMLEN - 1 < strlen(itemName)){
                        errorP("Item name %s len %d > max(%d)\n", itemName, (int)strlen(itemName), IH_NMLEN - 1);
                        ret = __LINE__; goto _exit;
                }
                pItemHeadInfo->magic = IH_MAGIC;
                pItemHeadInfo->size = itemSz;
                //imageSz += ITEM_HEAD_SZ;//not needed yet as all item_head moves to image header
                pItemHeadInfo->start = imageSz;
                pItemHeadInfo->next  =  (char*)(pItemHeadInfo + 1) - (char*)aAmlResImgHead;
                pItemHeadInfo->index = itemIndex;
                imageSz   += itemBodyOccupySz;
                pItemHeadInfo->nums   = totalItemNum;

                char* last_itemname = get_last_itemname(itemName);

                memcpy(pItemHeadInfo->name, last_itemname, strlen(last_itemname));
                debugP("pack item [%s]\n", last_itemname);
                ++pItemHeadInfo;//prepare for next item
                if (last_itemname != itemName) {
                    free(last_itemname);
                }

                fd_src = fopen(filePath, "rb");
                if(!fd_src){
                        errorP("Fail to open file [%s], strerror[%s]\n", filePath, strerror(errno));
                        ret = __LINE__; goto _exit;
                }
                for(size_t itemWriteLen = 0; itemWriteLen < itemSz; itemWriteLen += thisWriteLen)
                {
                        size_t leftLen = itemSz - itemWriteLen;

                        thisWriteLen = leftLen > BufSz ? BufSz : leftLen;
                        actualWriteLen = fread(itemBuf, 1, thisWriteLen, fd_src);
                        if(actualWriteLen != thisWriteLen){
                                errorP("Want to read 0x%x but actual 0x%x, at itemWriteLen 0x%x, leftLen 0x%x\n", 
                                                thisWriteLen, actualWriteLen, (unsigned)itemWriteLen, (unsigned)leftLen);
                                ret = __LINE__; goto _exit;
                        }
                        actualWriteLen = fwrite(itemBuf, 1, thisWriteLen, fd_dest);
                        if(actualWriteLen != thisWriteLen){
                                errorP("Want to write 0x%x but actual 0x%x\n", thisWriteLen, actualWriteLen);
                                ret = __LINE__; goto _exit;
                        }
                }
                fclose(fd_src), fd_src = NULL;
                memset(itemBuf, 0, itemStuffSz);
                thisWriteLen = itemStuffSz;
                actualWriteLen = fwrite(itemBuf, 1, thisWriteLen, fd_dest);
                if(actualWriteLen != thisWriteLen){
                        errorP("Want to write 0x%x, but 0x%x\n", thisWriteLen, actualWriteLen);
                        ret = __LINE__; goto _exit;
                }
        }
        (--pItemHeadInfo)->next = 0;

        //Create the header 
        aAmlResImgHead->version = AML_RES_IMG_VERSION_V2;
        memcpy(&aAmlResImgHead->magic[0], AML_RES_IMG_V1_MAGIC, AML_RES_IMG_V1_MAGIC_LEN);
        aAmlResImgHead->imgSz       = imageSz;
        aAmlResImgHead->imgItemNum  = totalItemNum;
        aAmlResImgHead->alignSz     = itemAlignSz;
        aAmlResImgHead->crc         = 0;
        //
        //Seek to file header to correct the header
        fseek(fd_dest, 0, SEEK_SET);
        actualWriteLen = fwrite(aAmlResImgHead, 1, HeadLen, fd_dest);
        if(actualWriteLen != HeadLen){
                errorP("Want to write 0x%x, but 0x%x\n", HeadLen, actualWriteLen);
                ret = __LINE__; goto _exit;
        }

        aAmlResImgHead->crc = calc_img_crc(fd_dest, 4);//Gen crc32
        fseek(fd_dest, 0, SEEK_SET);
        actualWriteLen = fwrite(&aAmlResImgHead->crc, 1, 4, fd_dest);
        if(4 != actualWriteLen){
                errorP("Want to write 4, but %d\n", actualWriteLen);
                ret = __LINE__; goto _exit;
        }

_exit:
        if(itemBuf) delete[] itemBuf, itemBuf = NULL;
        if(fd_src) fclose(fd_src), fd_src = NULL;
        if(fd_dest) fclose(fd_dest), fd_dest = NULL;
        return ret;
}

#ifdef BUILD_DLL
DLL_API
#endif// #ifdef BUILD_DLL
int res_img_pack(const char* szDir, const char* const outResImg)
{
    return _img_pack(&szDir, outResImg, traverse_dir, 0);
}

#ifndef BUILD_DLL 
static const char * const doc = "Amlogic `imgpack v3' usage:\n\
\n\
  # unpack files to the archive\n\
  imgpack -d [archive]  [destination-directory]\n\
  # pack files in directory to the archive\n\
  imgpack -r [source-directory]  [archive]\n\
  # pack files to the archive\n\
  imgpack  [file1].. [fileN]  [archive]\n";

int main(int argc, const char ** const argv)
{
        int ret = 0;
        int c = 0;
        const char* opt = argv[1];

        if(argc < 3) {
                printf("invalid arguments\n");
                printf("%s",doc);
                exit(-1);
        }

        if(!strcmp("?", opt))
        {
                printf("%s",doc);
                exit(-1);
        }
        if(!strcmp("-d", opt))//Unpack imgPath, dest-dir
        {
                ret = res_img_unpack(argv[2], argv[3], 1);
                exit(ret);
        }

        if(!strcmp("-r", opt))//pack: fileSrc, destImg, 
        {
                ret = res_img_pack(argv[2], argv[3]);
                exit(ret);
        }

        ret = _img_pack(&argv[1], argv[argc -1], get_file_path_from_argv, argc - 2);
        exit(ret);
}
#endif// #ifndef BUILD_DLL 
