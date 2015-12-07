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


//PIC_HD1080,PIC_HD720,PIC_VGA,PIC_QVGA

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
        printf("a=%s\n",a);
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
    VENC_GRP VencGrp;
    VENC_CHN VencChn;

    SAMPLE_RC_E enRcMode = SAMPLE_RC_CBR;
    HI_S32 s32ChnNum=1;
    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize;
    int socketfd,connfd;
    struct sockaddr_in servaddr;
    pid_t pid;
    char buff[1024];
    int pipefd[2];
    pthread_t tid;
    int ret;

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
    pid=fork();
    if (pid<0)
    {
        printf("fork failed!\n");
    }
    else if(pid>0)
    {
        memset(&stVbConf,0,sizeof(VB_CONF_S));

        stVbConf.u32MaxPoolCnt=128;

        u32BlkSize=SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,
            enSize,SAMPLE_PIXEL_FORMAT,SAMPLE_SYS_ALIGN_WIDTH);
        stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
        stVbConf.astCommPool[0].u32BlkCnt=10;

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
        VpssGrp = 0;
        VpssChn = 0;
        VencGrp = 0;
        VencChn = 0;
        while(1)
        {
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
            sleep(10);
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
    }
    else
    {
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
        while(1)
        {
            socklen_t len = sizeof(servaddr);
            connfd=accept(socketfd,(struct sockaddr*)&servaddr,&len);
            if(connfd==-1)
            {
                perror("accept failed");
                continue;
            }
            int n=recv(connfd,buff,sizeof(buff)/sizeof(char),0);
            buff[n]='\0';
            close(connfd);
            //printf("buff:%s\n",buff);
            if(!strcmp("abc123",buff))
            {
                if(-1==write(pipefd[1],"abc123",7))
                {
                    perror("write pipe error");
                }
                printf("write pipe\n");
                
            }
        }
        close(socketfd);
    }
    return 0;
}
