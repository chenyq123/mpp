#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "sample_comm.h"
#define JPEG_VENC 1
VIDEO_NORM_E gs_enNorm = VIDEO_ENCODING_MODE_NTSC;
/*
#define HD1080P 0
#define HD720P 1
#define VGA 2
#define QVGA 3
*/
//typedef enum PSIZE {HD1080P=0,HD720P,VGA,QVGA}PSIZE;

//PSIZE picsize=HD1080P;

PIC_SIZE_E enSize=PIC_HD1080;
int pipefd[2];
HI_BOOL g_bStopSignal = HI_FALSE;

//PIC_HD1080,PIC_HD720,PIC_VGA,PIC_QVGA

typedef struct  VENCPARA{
    VENC_GRP VencGrp;
    VENC_CHN VencChn;
}venc_para;

HI_S32 COMM_VENC_SnapProcess(VENC_GRP VencGrp, VENC_CHN VencChn)
{
    struct timeval TimeoutVal;
    fd_set read_fds;
    HI_S32 s32VencFd;
    VENC_CHN_STAT_S stStat;
    VENC_STREAM_S stStream;
    HI_S32 s32Ret;
    
    /******************************************
     step 1:  Regist Venc Channel to VencGrp
    ******************************************/
     s32Ret = HI_MPI_VENC_RegisterChn(VencGrp, VencChn);
     if (HI_SUCCESS != s32Ret)
     {
        SAMPLE_PRT("HI_MPI_VENC_RegisterChn faild with %#x!\n", s32Ret);
        return HI_FAILURE;
    }
    
    /******************************************
     step 2:  Start Recv Venc Pictures
    ******************************************/
     s32Ret = HI_MPI_VENC_StartRecvPic(VencChn);
     if (HI_SUCCESS != s32Ret)
     {
        SAMPLE_PRT("HI_MPI_VENC_StartRecvPic faild with%#x!\n", s32Ret);
        return HI_FAILURE;
    }
    /******************************************
     step 3:  recv picture
    ******************************************/
     while(!g_bStopSignal)
     {
      s32VencFd = HI_MPI_VENC_GetFd(VencChn);
      if (s32VencFd < 0)
      {
       SAMPLE_PRT("HI_MPI_VENC_GetFd faild with%#x!\n", s32VencFd);
       return HI_FAILURE;
   }

   FD_ZERO(&read_fds);
   FD_SET(s32VencFd, &read_fds);

   TimeoutVal.tv_sec  = 1000;
   TimeoutVal.tv_usec = 0;
      //s32Ret = select(s32VencFd+1, &read_fds, NULL, NULL, &TimeoutVal);
   s32Ret = select(s32VencFd+1, &read_fds, NULL, NULL, NULL);
   printf("get Frame!\n");
   if (s32Ret < 0) 
   {
      SAMPLE_PRT("snap select failed!\n");
      return HI_FAILURE;
  }
  else if (0 == s32Ret) 
  {
      SAMPLE_PRT("snap time out!\n");
      return HI_FAILURE;
  }
  else
  {
      if (FD_ISSET(s32VencFd, &read_fds))
      {
          s32Ret = HI_MPI_VENC_Query(VencChn, &stStat);
          if (s32Ret != HI_SUCCESS)
          {
              SAMPLE_PRT("HI_MPI_VENC_Query failed with %#x!\n", s32Ret);
              return HI_FAILURE;
          }

          stStream.pstPack = (VENC_PACK_S*)malloc(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
          if (NULL == stStream.pstPack)
          {
              SAMPLE_PRT("malloc memory failed!\n");
              return HI_FAILURE;
          }

          stStream.u32PackCount = stStat.u32CurPacks;
          s32Ret = HI_MPI_VENC_GetStream(VencChn, &stStream, HI_TRUE);
          if (HI_SUCCESS != s32Ret)
          {
              SAMPLE_PRT("HI_MPI_VENC_GetStream failed with %#x!\n", s32Ret);
              free(stStream.pstPack);
              stStream.pstPack = NULL;
              return HI_FAILURE;
          }

          s32Ret = SAMPLE_COMM_VENC_SaveSnap(&stStream);
          if (HI_SUCCESS != s32Ret)
          {
              SAMPLE_PRT("HI_MPI_VENC_GetStream failed with %#x!\n", s32Ret);
              free(stStream.pstPack);
              stStream.pstPack = NULL;
              return HI_FAILURE;
          }

          s32Ret = HI_MPI_VENC_ReleaseStream(VencChn, &stStream);
          if (s32Ret)
          {
              SAMPLE_PRT("HI_MPI_VENC_ReleaseStream failed with %#x!\n", s32Ret);
              free(stStream.pstPack);
              stStream.pstPack = NULL;
              return HI_FAILURE;
          }

          free(stStream.pstPack);
          stStream.pstPack = NULL;
      }
  }
}
    /******************************************
     step 3:  stop recv picture
    ******************************************/
     s32Ret = HI_MPI_VENC_StopRecvPic(VencChn);
     if (s32Ret != HI_SUCCESS)
     {
        SAMPLE_PRT("HI_MPI_VENC_StopRecvPic failed with %#x!\n",  s32Ret);
        return HI_FAILURE;
    }

    /******************************************
     step 4:  UnRegister
    ******************************************/
     s32Ret = HI_MPI_VENC_UnRegisterChn(VencChn);
     if (s32Ret != HI_SUCCESS)
     {
        SAMPLE_PRT("HI_MPI_VENC_UnRegisterChn failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

HI_VOID save_jpg(venc_para *para)
{

    COMM_VENC_SnapProcess(para->VencGrp,para->VencChn);

}

void thread_ChangePic(int fd)
{  
    char a[10];
    while(1)
    {
        printf("wait for read\n");
        read(fd,a,sizeof(char)*10);
        /*if(a=='1')
        {
            printf("PIC size had changed!\n");
            enSize = PIC_QVGA;
        }*/
            if(!strcmp(a,"abc123"))
            {
                enSize = PIC_QVGA;
            }
       // printf("a=%s\n",a);
        }
    }

    void signal_changePic()
    {
        printf("get signal!\n");
        enSize = PIC_QVGA;
    }

    void accept_thread(int socketfd)
    {
        while(1)
        {
            int connfd;
            char buff[512];
        //socklen_t len = sizeof(servaddr);
       // connfd=accept(socketfd,(struct sockaddr*)&servaddr,&len);
            connfd=accept(socketfd,NULL,NULL);
            if(connfd==-1)
            {
                perror("accept failed");
                continue;
            }
            int n=recv(connfd,buff,sizeof(buff)/sizeof(char),0);
            buff[n]='\0';
            close(connfd);
            if(!strcmp("abc123",buff))
            {
#if 0
            if(-1==write(pipefd[1],"abc123",7))
            {
                perror("write pipe error");
            }
            printf("write pipe\n");
#endif
            printf("send signal!\n");
            kill(getpid(),SIGUSR1);
        }
    }
}

int main()
{
    PAYLOAD_TYPE_E enPayLoad = PT_H264;
    VB_CONF_S stVbConf;
    SAMPLE_VI_CONFIG_S stViConfig;
    VPSS_GRP VpssGrp;
    VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stVpssGrpAttr;
    VPSS_CHN_ATTR_S stVpssChnAttr;
    VPSS_CHN_MODE_S stVpssChnMode;
    VPSS_EXT_CHN_ATTR_S stVpssExtChnAttr;
    VENC_GRP VencGrp,JpegVencGrp;
    VENC_CHN VencChn,JpegVencChn;
    VI_EXT_CHN_ATTR_S stExtChnAttr;
    VIDEO_FRAME_INFO_S stViFrameInfo;
    VI_CHN ExtChn = 1;
    SAMPLE_RC_E enRcMode = SAMPLE_RC_CBR;
    HI_S32 s32ChnNum=1;
    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize,JpegSize;
    int socketfd;
    struct sockaddr_in servaddr;
    pid_t pid,jpeg_pid;
    char buff[1024];
    venc_para  VencPara;
    pthread_t tid,tid2,JpegThread;
    int ret;

    signal(SIGUSR1,signal_changePic);
    if(pipe(pipefd) == -1)
    {
        perror("pipe failed");
        exit(0);
    }
    ret = pthread_create(&tid,NULL,thread_ChangePic,pipefd[0]);
    if(ret != 0)
    {
        perror("create thread error");
        exit(0);
    }

    memset(&stVbConf,0,sizeof(VB_CONF_S));

    stVbConf.u32MaxPoolCnt=128;

    u32BlkSize=SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,
        enSize,SAMPLE_PIXEL_FORMAT,SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt=10;
/*
    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,
        PIC_HD1080,SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt = 10;
*/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if(HI_SUCCESS!=s32Ret)
    {
        printf("system init failed with err code %#x!\n",s32Ret );
        goto END_1;
    }
    stViConfig.enViMode = SENSOR_TYPE;
    stViConfig.enRotate = ROTATE_NONE;
    stViConfig.enNorm = VIDEO_ENCODING_MODE_AUTO;
    stViConfig.enViChnSet = VI_CHN_SET_NORMAL;
    s32Ret=SAMPLE_COMM_VI_StartVi(&stViConfig);
    if(HI_SUCCESS!=s32Ret)
    {
        printf("start vi failed with err code %#x!\n",s32Ret);
        goto END_2;
    }

    stExtChnAttr.enPixFormat = SAMPLE_PIXEL_FORMAT;
    stExtChnAttr.s32BindChn = 0;
    stExtChnAttr.stDestSize.u32Width = 1920;
    stExtChnAttr.stDestSize.u32Height = 1080;
    stExtChnAttr.s32FrameRate = -1;
    stExtChnAttr.s32SrcFrameRate = -1;

    s32Ret = HI_MPI_VI_SetExtChnAttr(ExtChn, &stExtChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
       SAMPLE_PRT("HI_MPI_VI_SetExtChnAttr failed!\n");
       goto END_1;
    }
    //s32Ret = HI_MPI_VI_EnableChn(ViChn);
    

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm,enSize,&stSize);
    if(HI_SUCCESS!=s32Ret)
    {
        printf("SAMPLE_COMM_SYS_GetPicSize failed with err code %#x!\n",s32Ret);
        goto END_2;
    }

    VpssGrp = 0;
    stVpssGrpAttr.u32MaxW = stSize.u32Width;
    stVpssGrpAttr.u32MaxH = stSize.u32Height;
    stVpssGrpAttr.bDrEn = HI_FALSE;
    stVpssGrpAttr.bDbEn = HI_FALSE;
    stVpssGrpAttr.bIeEn = HI_TRUE;
    stVpssGrpAttr.bNrEn = HI_TRUE;
    stVpssGrpAttr.bHistEn = HI_TRUE;
    stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_AUTO;
    stVpssGrpAttr.enPixFmt = SAMPLE_PIXEL_FORMAT;
    s32Ret = SAMPLE_COMM_VPSS_StartGroup(VpssGrp,&stVpssGrpAttr);
    if(HI_SUCCESS != s32Ret)
    {
        printf("SAMPLE_COMM_VPSS_StartGroup failed with err code %#x!\n",s32Ret);
        goto END_3;
    }
    s32Ret = SAMPLE_COMM_VI_BindVpss(stViConfig.enViMode);
    if(HI_SUCCESS != s32Ret)
    {
        printf("SAMPLE_COMM_vi_BindVpss failed with err code %#x\n",s32Ret);
        goto END_4;
    }
    VpssChn=0;
    memset(&stVpssChnAttr,0,sizeof(stVpssChnAttr));
    stVpssChnAttr.bFrameEn =HI_FALSE;
    stVpssChnAttr.bSpEn = HI_TRUE;
    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp,VpssChn,&stVpssChnAttr,HI_NULL,HI_NULL);
    if(HI_SUCCESS != s32Ret)
    {
        printf("SAMPLE_COMM_VPSS_EnableChn failed with err code %#x\n",s32Ret);
        goto END_5;
    }
    
    s32Ret = HI_MPI_VI_EnableChn(ExtChn);
    if (HI_SUCCESS != s32Ret)
    {
       SAMPLE_PRT("HI_MPI_VI_EnableChn failed!\n");
       
    }

    socketfd=socket(AF_INET,SOCK_STREAM,0);
    if(socketfd==-1)
    {
        perror("socket failed");
        exit(0);
    }
    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(8000);
    if(bind(socketfd,(struct sockaddr*)&servaddr,sizeof(servaddr))==-1)
    {
        perror("bind failed");
        exit(0);
    }
    if(listen(socketfd,10)==-1)
    {
        perror("listen failed");
        exit(0);
    }
    ret = pthread_create(&tid2,NULL,accept_thread,socketfd);
    if(ret != 0)
    {
        perror("create accept_thread error");
        exit(0);
    }
    s32Ret = HI_MPI_VI_SetFrameDepth(ExtChn, 2);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("set Frame depth err:0x%x\n", s32Ret);
    }
#if JPEG_VENC
    JpegSize.u32Width = 1920;
    JpegSize.u32Height = 1080;
    JpegVencChn=1;
    JpegVencGrp=1;
    s32Ret = SAMPLE_COMM_VENC_SnapStart(JpegVencGrp,JpegVencChn,&JpegSize);
    if(HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start snap failed!\n");
    }
    VencPara.VencChn=JpegVencChn;
    VencPara.VencGrp=JpegVencGrp;
    jpeg_pid = pthread_create(&JpegThread,0,(HI_VOID*)save_jpg,&VencPara);

#endif
    VpssGrp = 0;
    VpssChn = 0;
    VencGrp = 0;
    VencChn = 0;
    while(1)
    {
        s32Ret = HI_MPI_VI_GetFrameTimeOut(ExtChn,&stViFrameInfo, 0);
        if(HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("HI_MPI_VI_GetFrame failed!\n");
        }
        s32Ret=HI_MPI_VENC_SendFrame(1,&stViFrameInfo);
        if(HI_SUCCESS!=s32Ret)
        {
          printf("HI_MPI_VENC_SendFrame fail with err code %#x!\n",s32Ret);
      }
           // printf("VENC...\n");
      printf("enSize:%d\n",enSize);
            //enSize=PIC_VGA;
      s32Ret = SAMPLE_COMM_VENC_Start(VencGrp,VencChn,enPayLoad,
        gs_enNorm,enSize,enRcMode);
      if(HI_SUCCESS != s32Ret)
      {
        printf("SAMPLE_COMM_VENC_Start failed with err code %#x\n",s32Ret);
        goto END_6;
        }
        s32Ret = SAMPLE_COMM_VENC_BindVpss(VencGrp,VpssGrp,VpssChn);
        if(HI_SUCCESS != s32Ret)
        {
            printf("SAMPLE_COMM_VENC_BindVpss failed with err code %#x\n",s32Ret);
            goto END_6;
        }
        s32Ret = SAMPLE_COMM_VENC_StartGetStream(s32ChnNum);
        if(HI_SUCCESS != s32Ret)
        {
            printf("SAMPLE_COMM_VENC_StartGetStream failed with err code %#x\n",s32Ret);
            goto END_6;
        }
        int sret = sleep(10);
        printf("sret:%d\n",sret);
        while(sret!=0)
        {
            sret=sleep(sret);
        }
        SAMPLE_COMM_VENC_StopGetStream();
        END_6:
        VpssGrp = 0;
        VpssChn = 0;
        VencGrp = 0;   
        VencChn = 0;
        SAMPLE_COMM_VENC_UnBindVpss(VencGrp, VpssGrp, VpssChn);
        SAMPLE_COMM_VENC_Stop(VencGrp,VencChn);
    }
END_5:
VpssGrp=0;
VpssChn=0;
SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
END_4:
SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
END_3:
SAMPLE_COMM_VPSS_StopGroup(VpssGrp);
END_2:
SAMPLE_COMM_VI_StopVi(&stViConfig);
END_1:
SAMPLE_COMM_SYS_Exit();

close(socketfd);

return 0;
}
