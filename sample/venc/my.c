#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sample_comm.h"
#include "mpi_ive.h"
#include "hi_comm_ive.h"
HI_S32 INIT_MPP_SYS(VB_CONF_S *pstVbConf)
{
	MPP_SYS_CONF_S stSysConf={0};
	HI_S32 s32Ret = HI_FAILURE;
	HI_MPI_SYS_Exit();
	HI_MPI_VB_Exit();
	if(NULL == pstVbConf)
	{
		printf("input parameter is null,it is invaild\n");
		return HI_FAILURE;
	}

	s32Ret=HI_MPI_VB_SetConf(pstVbConf);
	if(HI_SUCCESS!=s32Ret)
	{
		printf("HI_MPI_VE_SetConf failed!\n");
		return HI_FAILURE;
	}

	s32Ret=HI_MPI_VB_Init();
	if(HI_SUCCESS!=s32Ret)
	{
		printf("HI_MPI_VB_Init failed! with err code %#x!\n",s32Ret);
		return s32Ret;
	}

	stSysConf.u32AlignWidth=SAMPLE_SYS_ALIGN_WIDTH;
	s32Ret=HI_MPI_SYS_SetConf(&stSysConf);
	if(HI_SUCCESS!=s32Ret)
	{
		printf("HI_MPI_SYS_SetConf failed\n");
		return HI_FAILURE;
	}
	s32Ret=HI_MPI_SYS_Init();
	if(HI_SUCCESS!=s32Ret)
	{
		printf("HI_MPI_SYS_Init failed!\n");
		return HI_FAILURE;
	}
	return HI_SUCCESS;
}
HI_S32 StartVi(VI_DEV ViDev,
			VI_CHN ViChn,
			VI_DEV_ATTR_S *stDevAttr,
			VI_CHN_ATTR_S *stChnAttr){
	HI_S32 s32Ret;
	s32Ret=SAMPLE_COMM_ISP_SensorInit();
	if(HI_SUCCESS!=s32Ret)
	{
		printf("%s: Sensor init failed!\n",__FUNCTION__);
		return HI_FAILURE;
	}
	s32Ret=HI_MPI_VI_SetDevAttr(ViDev,stDevAttr);
	if(s32Ret !=HI_SUCCESS)
	{
		printf("Set dev attributes failed with error code %#x!\n",s32Ret);
		return HI_FAILURE;
	}
	s32Ret=HI_MPI_VI_EnableDev(ViDev);
	if(s32Ret != HI_SUCCESS)
	{
		printf("Enable dev failed with error code %#x!\n",s32Ret);
		return HI_FAILURE;
	}

	s32Ret = SAMPLE_COMM_ISP_Run();
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("%s: ISP init failed!\n", __FUNCTION__);
	/* disable videv */
        return HI_FAILURE;
    }

	s32Ret=HI_MPI_VI_SetChnAttr(ViChn,stChnAttr);
	if(s32Ret !=HI_SUCCESS)
	{
		printf("Set Chn attributes failed with error code %#x!\n",s32Ret);
		return HI_FAILURE;
	}
	s32Ret=HI_MPI_VI_EnableChn(ViChn);
	if(s32Ret != HI_SUCCESS)
	{
		printf("Enable chn failed with error code %#x!\n",s32Ret);
		return HI_FAILURE;
	}
	return HI_SUCCESS;
}
HI_S32 VPSS_StartGroup_VI_BindVpss(VPSS_GRP VpssGrp,VPSS_GRP_ATTR_S *pstVpssGrpAttr)
{
	HI_S32 s32Ret = HI_FAILURE;
	VPSS_GRP_PARAM_S stVpssParam;
	MPP_CHN_S stSrcChn;
	MPP_CHN_S stDestChn;
	memset(&stSrcChn,0,sizeof(stSrcChn));
	memset(&stDestChn,0,sizeof(stDestChn));
	if(VpssGrp<0||VpssGrp > VPSS_MAX_GRP_NUM)
	{
		printf("VpssGrp%d is out of rang.\n",VpssGrp);
		return HI_FAILURE;
	}
	if (HI_NULL == pstVpssGrpAttr)
    {
        printf("null ptr,line%d. \n", __LINE__);
        return HI_FAILURE;
    }

    s32Ret =HI_MPI_VPSS_CreateGrp(VpssGrp,pstVpssGrpAttr);
    if(s32Ret !=HI_SUCCESS)
    {
    	printf("HI_MPI_VPSS_CreateGrp failed with %#x\n",s32Ret);
    	return HI_FAILURE;
    }
    s32Ret=HI_MPI_VPSS_GetGrpParam(VpssGrp,&stVpssParam);
    if(s32Ret!=HI_SUCCESS)
    {
    	printf("failed with %#x!\n",s32Ret);
    	return HI_FAILURE;
    }
    stVpssParam.u32MotionThresh=0;
    s32Ret=HI_MPI_VPSS_SetGrpParam(VpssGrp,&stVpssParam);
    if(s32Ret!=HI_SUCCESS)
    {
    	printf("HI_MPI_VPSS_SetGrpParam failed with %#x\n",s32Ret);
    	return HI_FAILURE;	
    }
    s32Ret=HI_MPI_VPSS_StartGrp(VpssGrp);
    if(s32Ret!=HI_SUCCESS)
    {
    	printf("HI_MPI_VPSS_StartGrp failed with %#x\n",s32Ret);
    	return HI_FAILURE;
    }
    stSrcChn.enModId=HI_ID_VIU;
    stSrcChn.s32DevId=0;
    stSrcChn.s32ChnId=0;

    stDestChn.enModId=HI_ID_VPSS;
    stDestChn.s32DevId=VpssGrp;
    stDestChn.s32ChnId=0;

    s32Ret=HI_MPI_SYS_Bind(&stSrcChn,&stDestChn);
    if(s32Ret !=HI_SUCCESS)
    {
    	printf("failed with %#x!\n", s32Ret);
    	return HI_FAILURE;
    }
    return HI_SUCCESS;
}	
HI_S32 COMM_VPSS_EnableChn(VPSS_GRP VpssGrp,VPSS_CHN VpssChn,
				VPSS_CHN_ATTR_S *pstVpssChnAttr,
				VPSS_CHN_MODE_S *pstVpssChnMode,
				VPSS_EXT_CHN_ATTR_S *pstVpssExtChnAttr)
{
	 HI_S32 s32Ret;

    if (VpssGrp < 0 || VpssGrp > VPSS_MAX_GRP_NUM)
    {
        printf("VpssGrp%d is out of rang[0,%d]. \n", VpssGrp, VPSS_MAX_GRP_NUM);
        return HI_FAILURE;
    }

    if (VpssChn < 0 || VpssChn > VPSS_MAX_CHN_NUM)
    {
        printf("VpssChn%d is out of rang[0,%d]. \n", VpssChn, VPSS_MAX_CHN_NUM);
        return HI_FAILURE;
    }

    if (HI_NULL == pstVpssChnAttr && HI_NULL == pstVpssExtChnAttr)
    {
        printf("null ptr,line%d. \n", __LINE__);
        return HI_FAILURE;
    }

    if (VpssChn < VPSS_MAX_PHY_CHN_NUM)
    {
        s32Ret = HI_MPI_VPSS_SetChnAttr(VpssGrp, VpssChn, pstVpssChnAttr);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_PRT("HI_MPI_VPSS_SetChnAttr failed with %#x\n", s32Ret);
            return HI_FAILURE;
        }
    }
    else
    {
        s32Ret = HI_MPI_VPSS_SetExtChnAttr(VpssGrp, VpssChn, pstVpssExtChnAttr);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_PRT("%s failed with %#x\n", __FUNCTION__, s32Ret);
            return HI_FAILURE;
        }
    }
    
    if (VpssChn < VPSS_MAX_PHY_CHN_NUM && HI_NULL != pstVpssChnMode)
    {
        s32Ret = HI_MPI_VPSS_SetChnMode(VpssGrp, VpssChn, pstVpssChnMode);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_PRT("%s failed with %#x\n", __FUNCTION__, s32Ret);
            return HI_FAILURE;
        }     
    }
    
    s32Ret = HI_MPI_VPSS_EnableChn(VpssGrp, VpssChn);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("HI_MPI_VPSS_EnableChn failed with %#x\n", s32Ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}
HI_S32 COMM_VENC_Start(VENC_GRP VencGrp,
		VENC_CHN VencChn,
		VENC_CHN_ATTR_S *stAttr,
		SIZE_S *stPicSize)
{
	HI_S32  s32Ret;
	MPP_CHN_S stSrcChn,stDestChn;
	VENC_CHN_ATTR_S stVencChnAttr;

	memset(&stVencChnAttr,0,sizeof(stVencChnAttr));
	s32Ret=HI_MPI_VENC_CreateGroup(VencGrp);
	if(HI_SUCCESS!=s32Ret)
	{
		printf("HI_MPI_VENC_CreateGroup[%d] failed with %#x!\n",VencGrp,s32Ret);
		return HI_FAILURE;
	}
	stAttr->stVeAttr.enType=PT_H264;
	stAttr->stVeAttr.stAttrH264e.u32PicWidth=stPicSize->u32Width;
	stAttr->stVeAttr.stAttrH264e.u32PicHeight=stPicSize->u32Height;
	stAttr->stVeAttr.stAttrH264e.u32MaxPicWidth=stPicSize->u32Width;
	stAttr->stVeAttr.stAttrH264e.u32MaxPicHeight=stPicSize->u32Height;
	stAttr->stVeAttr.stAttrH264e.u32BufSize=stPicSize->u32Width*stPicSize->u32Height*2;
	stAttr->stVeAttr.stAttrH264e.u32Profile=0;
	stAttr->stVeAttr.stAttrH264e.bByFrame=HI_TRUE;
	stAttr->stVeAttr.stAttrH264e.bField=HI_FALSE;
	stAttr->stVeAttr.stAttrH264e.bMainStream=HI_TRUE;
	stAttr->stVeAttr.stAttrH264e.u32Priority=0;
	stAttr->stVeAttr.stAttrH264e.bVIField=HI_FALSE;

	stAttr->stRcAttr.enRcMode=VENC_RC_MODE_H264CBR;
	stAttr->stRcAttr.stAttrH264Cbr.u32Gop=25;
	stAttr->stRcAttr.stAttrH264Cbr.u32StatTime=1;
	stAttr->stRcAttr.stAttrH264Cbr.u32ViFrmRate=25;
	stAttr->stRcAttr.stAttrH264Cbr.fr32TargetFrmRate=25;
	stAttr->stRcAttr.stAttrH264Cbr.u32BitRate=1024*4;

	VencChn=0;
	s32Ret=HI_MPI_VENC_CreateChn(VencChn,stAttr);
	if(HI_SUCCESS!=s32Ret)
	{
		printf("HI_MPI_VENC_CreateChn[%d] failed with %#x!\n",VencChn,s32Ret);
		return s32Ret;
	}

	s32Ret=HI_MPI_VENC_RegisterChn(VencGrp,VencChn);
	if(HI_SUCCESS!=s32Ret)
	{
		printf("HI_MPI_VENC_RegisterChn fail with %#x!\n",s32Ret);
		return s32Ret;
	}

	s32Ret=HI_MPI_VENC_StartRecvPic(VencChn);
	if(HI_SUCCESS!=s32Ret)
	{
		printf("HI_MPI_VENC_StartRecvPic fail with %#x!\n",s32Ret);
		return HI_FAILURE;
	}

	memset(&stSrcChn,0,sizeof(stSrcChn));
	memset(&stDestChn,0,sizeof(stDestChn));
	stSrcChn.enModId=HI_ID_VPSS;
	stSrcChn.s32DevId=0;
	stSrcChn.s32ChnId=0;

	stDestChn.enModId=HI_ID_GROUP;
	stDestChn.s32DevId=0;
	stDestChn.s32ChnId=0;

	s32Ret=HI_MPI_SYS_Bind(&stSrcChn,&stDestChn);
	if(s32Ret!=HI_SUCCESS)
	{
		printf("HI_MPI_SYS_Bind err 0x%x\n",s32Ret);
		return s32Ret;
	}	
	return HI_SUCCESS;
}

HI_S32 COMM_VENC_StartGetStream()
{
	HI_S32 s32Ret;
	VENC_CHN VencChn=0;
	VENC_CHN_ATTR_S stVencChnAttr;
	FILE *pFile;
	HI_S32 VencFd;
	struct timeval TimeoutVal;
	fd_set read_fds;
	HI_CHAR aszFileName[64];
	VENC_CHN_STAT_S stStat;
	VENC_STREAM_S stStream;

	VIDEO_FRAME_INFO_S pstVideoFrame;

	HI_CHAR pcMmzName[64]={};
	MPP_CHN_S chn;
	chn.enModId=HI_ID_VPSS;
	chn.s32DevId=0;
	chn.s32ChnId=0;


	s32Ret=HI_MPI_VENC_GetChnAttr(VencChn,&stVencChnAttr);
	if(s32Ret!=HI_SUCCESS)
	{
		printf("HI_MPI_VENC_GetChnAttr chn[%d] failed with %#x!\n",VencChn,s32Ret);
		return s32Ret;
	}
	sprintf(aszFileName,"stream_chn%d.h264",0);
	pFile=fopen(aszFileName,"wb");
	if(!pFile)
	{
		printf("open file %s failed!\n",aszFileName);
		return NULL;
	}
	VencFd=HI_MPI_VENC_GetFd(0);
	if(VencFd<0)
	{
		printf("HI_MPI_VENC_GetFd failed with %#x!\n",VencFd);
		return NULL;
	}
	int a=600000;
	while(a!=0){
		a--;
		FD_ZERO(&read_fds);
		FD_SET(VencFd,&read_fds);
		TimeoutVal.tv_sec=2;
		TimeoutVal.tv_usec=0;


		s32Ret=select(VencFd+1,&read_fds,NULL,NULL,&TimeoutVal);
		if(s32Ret<0)
		{
			printf("select failed!\n");
			break;
		}
		else if(s32Ret == 0)
		{
			printf("get venc stream time out,exit thread\n");
			continue;
		}
		else
		{
			FD_ISSET(VencFd,&read_fds);
			memset(&stStream,0,sizeof(stStream));
			s32Ret=HI_MPI_VENC_Query(0,&stStat);
			if(HI_SUCCESS!=s32Ret)
			{
				printf("HI_MPI_VENC_Query chn failed with %#x!\n",s32Ret);
				break;
			}

			stStream.pstPack=(VENC_PACK_S*)malloc(sizeof(VENC_PACK_S)*stStat.u32CurPacks);
			if(NULL==stStream.pstPack)
			{
				printf("malloc stream pack failed!\n");
				break;
			}
			stStream.u32PackCount=stStat.u32CurPacks;
			s32Ret=HI_MPI_VENC_GetStream(0,&stStream,HI_TRUE);
			if(HI_SUCCESS!=s32Ret)
			{
				free(stStream.pstPack);
				stStream.pstPack=NULL;
				printf("HI_MPI_VENC_GETStream failed with %#x!\n",s32Ret);
				break;
			}
			HI_S32 i=0;
			for(i=0;i<stStream.u32PackCount;i++)
			{
				fwrite(stStream.pstPack[i].pu8Addr[0],
					stStream.pstPack[i].u32Len[0],1,pFile);
				fflush(pFile);
				if(stStream.pstPack[i].u32Len[1]>0)
				{
					fwrite(stStream.pstPack[i].pu8Addr[1],
						stStream.pstPack[i].u32Len[1],1,pFile);
					fflush(pFile);
				}
			}

			s32Ret=HI_MPI_VENC_ReleaseStream(0,&stStream);
			if(HI_SUCCESS!=s32Ret)
			{
				free(stStream.pstPack);
				stStream.pstPack=NULL;
				break;
			}
			free(stStream.pstPack);
			stStream.pstPack=NULL;
		}
	//	fclose(pFile);
	}
		fclose(pFile);
}

int main()
{
	VB_CONF_S stVbConf;
	HI_S32 s32Ret=HI_SUCCESS;
	VI_DEV ViDev = 0;
	VI_CHN ViChn = 0;
	VI_DEV_ATTR_S stDevAttr;
	VI_CHN_ATTR_S stChnAttr;
	VPSS_GRP VpssGrp;
	VPSS_GRP_ATTR_S stVpssGrpAttr;
	VPSS_CHN_ATTR_S stVpssChnAttr;
	VPSS_CHN_MODE_S stVpssChnMode;
	VPSS_CHN VpssChn;
	VENC_GRP VencGrp;
	VENC_CHN VencChn;
	SIZE_S stPicSize;
	VENC_CHN_ATTR_S stAttr;
//	VIDEO_FRAME_INFO_S pstFrameInfo;
	//初始化mpp系统
	memset(&stVbConf,0,sizeof(VB_CONF_S));
	stVbConf.u32MaxPoolCnt=128;
	stVbConf.astCommPool[0].u32BlkSize = 1980*1080*2;
	stVbConf.astCommPool[0].u32BlkCnt = 10;
	//memset(stVbConf.astCommPool[0].acMmzName,0,
	//		sizeof(stVbConf.astCommPool[0].acMmzName));
//	strcpy(stVbConf.astCommPool[0].acMmzName,"DDR1");
//	printf("%s\n",stVbConf.astCommPool[0].acMmzName);
	//s32Ret=SAMPLE_COMM_SYS_Init(&stVbConf);
	s32Ret=INIT_MPP_SYS(&stVbConf);
	if(HI_SUCCESS!=s32Ret)
	{
		printf("system init failed with %d!\n",s32Ret);
		goto END_VENC_720P_CLASSIC_0;
	}
	memset(&stDevAttr,0,sizeof(stDevAttr));
	memset(&stChnAttr,0,sizeof(stChnAttr));
	stDevAttr.enIntfMode=VI_MODE_DIGITAL_CAMERA;
	stDevAttr.enWorkMode=VI_WORK_MODE_1Multiplex;
	stDevAttr.au32CompMask[0]=0xFF000000;
	stDevAttr.au32CompMask[1]=0x0;
	stDevAttr.enScanMode=VI_SCAN_PROGRESSIVE;
	stDevAttr.s32AdChnId[0]=-1;
	stDevAttr.s32AdChnId[1]=-1;
	stDevAttr.s32AdChnId[2]=-1;
	stDevAttr.s32AdChnId[3]=-1;
	stDevAttr.enDataSeq=VI_INPUT_DATA_YUYV;
	/*stDevAttr.stSynCfg={
		 VI_VSYNC_PULSE, 
		 VI_VSYNC_NEG_HIGH, 
		 VI_HSYNC_VALID_SINGNAL,
		 VI_HSYNC_NEG_HIGH,
		 VI_VSYNC_NORM_PULSE,
		 VI_VSYNC_VALID_NEG_HIGH,
    {0,            1920,        0,
     0,            1080,        0,
     0,            0,            0}
	};*/
    stDevAttr.stSynCfg.enVsync=VI_VSYNC_PULSE;
    stDevAttr.stSynCfg.enVsyncNeg=VI_VSYNC_NEG_HIGH;
    stDevAttr.stSynCfg.enHsync=VI_HSYNC_VALID_SINGNAL;
    stDevAttr.stSynCfg.enHsyncNeg=VI_HSYNC_NEG_HIGH;
    stDevAttr.stSynCfg.enVsyncValid=VI_VSYNC_NORM_PULSE;
    stDevAttr.stSynCfg.enVsyncValidNeg=VI_VSYNC_VALID_NEG_HIGH;
 	/*
 	stDevAttr.stSynCfg.stTimingBlank={
    	0,            1920,        0,
     	0,            1080,        0,
     	0,            0,           0
    };
	*/
	#if 0
	int a[3][3]={
		0,            1920,        0,
     	0,            1080,        0,
     	0,            0,           0
	};
	memcpy(&stDevAttr.stSynCfg.stTimingBlank,a,sizeof(a));
	//stDevAttr.enDataPath=VI_PATH_ISP;
	#endif
	//stDevAttr.enDataPath=VI_PATH_BYPASS;
	stDevAttr.enDataPath=VI_PATH_ISP;
	stDevAttr.enInputDataType=VI_DATA_TYPE_RGB;




	stChnAttr.stCapRect.s32X=0;
	stChnAttr.stCapRect.s32Y=0;
	stChnAttr.stCapRect.u32Width=1920;
	stChnAttr.stCapRect.u32Height=1080;
	stChnAttr.stDestSize.u32Width=1920;
	stChnAttr.stDestSize.u32Height=1080;
	stChnAttr.enCapSel=VI_CAPSEL_BOTH;
	stChnAttr.enPixFormat=PIXEL_FORMAT_YUV_SEMIPLANAR_420;
	stChnAttr.bMirror=HI_FALSE;
	stChnAttr.bFlip=HI_TRUE;
	stChnAttr.bChromaResample=HI_FALSE;
	stChnAttr.s32SrcFrameRate=30;
	stChnAttr.s32FrameRate=30;
	
	s32Ret=StartVi(ViDev, ViChn,&stDevAttr,&stChnAttr);
	if(s32Ret!=HI_SUCCESS)
	{
		printf("StartVi failed with error code %#x!\n",s32Ret);
		return HI_FAILURE;
	}

#if 1

	HI_U32 u32Depth = 5;
	s32Ret = HI_MPI_VI_SetFrameDepth(ViChn, u32Depth);
	if (HI_SUCCESS != s32Ret)
	{
		printf("set max depth err:0x%x\n", s32Ret);
		return s32Ret;
	}

	while(1){
		IVE_HANDLE plveHandle;
		IVE_SRC_INFO_S pstSrc;
		IVE_MEM_INFO_S pstDst;
		IVE_THRESH_CTRL_S pstThreshCtrl;
		VIDEO_FRAME_INFO_S pstFrameInfo;
		
		HI_VOID *pVirDst;

		memset(&pstSrc,0,sizeof(IVE_SRC_INFO_S));
		memset(&pstDst,0,sizeof(IVE_MEM_INFO_S));
		memset(&pstThreshCtrl,0,sizeof(IVE_THRESH_CTRL_S));

		s32Ret=HI_MPI_VI_GetFrame(0,  &pstFrameInfo);
		if(s32Ret!=HI_SUCCESS)
		{
			printf("HI_MPI_VI_GetFrame fail with err code %#x!\n",s32Ret);
		}
		else{
			pstThreshCtrl.enOutFmt=IVE_THRESH_OUT_FMT_BINARY;
			pstThreshCtrl.u32Thresh=30;
			pstThreshCtrl.u32MinVal=0;
			pstThreshCtrl.u32MaxVal=255;

			pstSrc.enSrcFmt=IVE_SRC_FMT_SINGLE;
			pstSrc.u32Height=1920;
			pstSrc.u32Width=1080;
			pstSrc.stSrcMem.u32PhyAddr=pstFrameInfo.stVFrame.u32PhyAddr[1];
			pstSrc.stSrcMem.u32Stride=8;
			printf("u32Stride:%d\n",pstFrameInfo.stVFrame.u32Stride[2]);
			printf("u32PhyAddr:%#x\n",pstFrameInfo.stVFrame.u32PhyAddr[2]);
			#if 0
			s32Ret=HI_MPI_SYS_MmzAlloc_Cached(&pstDst.u32PhyAddr,&pVirDst,NULL,NULL,1920*8);
			if(HI_SUCCESS!=s32Ret)
			{
				printf("HI_MPI_SYS_MmzAlloc_Cached fail with err code %#x!",s32Ret);
				return HI_FAILURE;
			}
			pstDst.u32Stride=8;
			printf("get frame!\n");
			s32Ret=HI_MPI_IVE_THRESH(&plveHandle,&pstSrc,&pstDst,&pstThreshCtrl,HI_TRUE);
			if(HI_SUCCESS!=s32Ret)
			{
				printf("HI_MPI_IVE_THRESH fail with error code %#x!",s32Ret);
				return HI_FAILURE;
			}
			HI_MPI_VI_ReleaseFrame(ViChn, &pstFrameInfo);
			HI_MPI_SYS_MmzFree(&pstDst.u32PhyAddr,pVirDst);
			#endif
			HI_MPI_VI_ReleaseFrame(ViChn, &pstFrameInfo);
		}
	//	HI_MPI_VI_ReleaseFrame(ViChn, &stFrame);
	}
#endif
//	HI_MPI_VI_ReleaseFrame(ViChn, &stFrame);
	VpssGrp=0;
	stVpssGrpAttr.u32MaxW = 1920;
    stVpssGrpAttr.u32MaxH = 1080;
    stVpssGrpAttr.bDrEn = HI_FALSE;
    stVpssGrpAttr.bDbEn = HI_FALSE;
    stVpssGrpAttr.bIeEn = HI_TRUE;
    stVpssGrpAttr.bNrEn = HI_TRUE;
    stVpssGrpAttr.bHistEn = HI_TRUE;
    stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_AUTO;
    stVpssGrpAttr.enPixFmt = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    s32Ret=VPSS_StartGroup_VI_BindVpss(VpssGrp,&stVpssGrpAttr);
    if(HI_SUCCESS!=s32Ret)
    {
    	printf("Start Vpss failed with error code %#x!\n",s32Ret);
    	goto END_VENC_720P_CLASSIC_2;
    }
    VpssChn=0;
    memset(&stVpssChnAttr,0,sizeof(stVpssChnAttr));
    stVpssChnAttr.bFrameEn=HI_FALSE;
    stVpssChnAttr.bSpEn=HI_TRUE;
#if 0
    s32Ret = HI_MPI_VPSS_GetChnMode(VpssGrp,VpssChn,&stVpssChnMode);
	if(s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VPSS_GetChnMode fail with err code %#x!\n",s32Ret);
	}
    stVpssChnMode.enChnMode = VPSS_CHN_MODE_USER;
    stVpssChnMode.bDouble = HI_FALSE;
    stVpssChnMode.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    stVpssChnMode.u32Width = 1920;
    stVpssChnMode.u32Height = 1080;
#endif
  //  s32Ret=COMM_VPSS_EnableChn(VpssGrp,VpssChn,&stVpssChnAttr,&stVpssChnMode,HI_NULL);
    s32Ret=COMM_VPSS_EnableChn(VpssGrp,VpssChn,&stVpssChnAttr,HI_NULL,HI_NULL);
    if(HI_SUCCESS!=s32Ret)
    {
    	printf("Enable vps chn failed! with error code %#x!\n",s32Ret);
    	goto END_VENC_720P_CLASSIC_4;
    }

    VencGrp=0;
    VencChn=0;
    stPicSize.u32Width=1920;
    stPicSize.u32Height=1080;
    memset(&stAttr,0,sizeof(stAttr));
    s32Ret=COMM_VENC_Start(VencGrp,VencChn,&stAttr,&stPicSize);
    if(HI_SUCCESS!=s32Ret)
    {
    	printf("COMM_VENC_Start fail with error code %#x!\n",s32Ret);
    	goto END_VENC_720P_CLASSIC_5;
    }
    COMM_VENC_StartGetStream();
END_VENC_720P_CLASSIC_5:
    VpssGrp = 0;
    
    VpssChn = 0;
    VencGrp = 0;   
    VencChn = 0;
    SAMPLE_COMM_VENC_UnBindVpss(VencGrp, VpssGrp, VpssChn);
    SAMPLE_COMM_VENC_Stop(VencGrp,VencChn);

   // SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
END_VENC_720P_CLASSIC_4:	//vpss stop
    VpssGrp = 0;
    
    VpssChn = 0;
    SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
   
END_VENC_720P_CLASSIC_3:    //vpss stop       
 //   SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
END_VENC_720P_CLASSIC_2:    //vpss stop   
    SAMPLE_COMM_VPSS_StopGroup(VpssGrp);
END_VENC_720P_CLASSIC_1:	//vi stop
 //   SAMPLE_COMM_VI_StopVi(&stViConfig);
END_VENC_720P_CLASSIC_0:	//system exit
    SAMPLE_COMM_SYS_Exit();

}
