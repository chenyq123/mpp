#include "../extdrv/tw2865/tw2865.h"
#include "hi_common.h"
#include "hi_comm_sys.h"
#include "hi_comm_vb.h"
#include "hi_comm_isp.h"
#include "hi_comm_vi.h"
#include "hi_comm_vo.h"
#include "hi_comm_venc.h"
#include "hi_comm_vpss.h"
#include "hi_comm_vdec.h"
#include "hi_comm_vda.h"
#include "hi_comm_region.h"
#include "hi_comm_adec.h"
#include "hi_comm_aenc.h"
#include "hi_comm_ai.h"
#include "hi_comm_ao.h"
#include "hi_comm_aio.h"
#include "hi_comm_isp.h"
#include "hi_defines.h"

#include "mpi_sys.h"
#include "mpi_vb.h"
#include "mpi_vi.h"
#include "mpi_vo.h"
#include "mpi_venc.h"
#include "mpi_vpss.h"
#include "mpi_vdec.h"
#include "mpi_vda.h"
#include "mpi_region.h"
#include "mpi_adec.h"
#include "mpi_aenc.h"
#include "mpi_ai.h"
#include "mpi_ao.h"
#include "mpi_isp.h"

#include "hi_sns_ctrl.h"
#include <stdio.h>
#include <string.h>
#include <cv.h>
#include <cxcore.h>
#include <highgui.h>
#include "opencv2/opencv.hpp"

VI_DEV ViDev = 0;
VI_CHN ViChn = 0;
VI_CHN ExtChn = 1;

/*imx122 DC 12bit输入*/
VI_DEV_ATTR_S DEV_ATTR_IMX122_DC_1080P =
{
    /*接口模式*/
    VI_MODE_DIGITAL_CAMERA,
    /*1、2、4路工作模式*/
    VI_WORK_MODE_1Multiplex,
    /* r_mask    g_mask    b_mask*/
    {0xFFF00000,    0x0},
    /*逐行or隔行输入*/
    VI_SCAN_PROGRESSIVE,
    /*AdChnId*/
    {-1, -1, -1, -1},
    /*enDataSeq, 仅支持YUV格式*/
    VI_INPUT_DATA_YUYV,

    /*同步信息，对应reg手册的如下配置, --bt1120时序无效*/
    {
    /*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
    VI_VSYNC_PULSE, VI_VSYNC_NEG_HIGH, VI_HSYNC_VALID_SINGNAL,VI_HSYNC_NEG_HIGH,VI_VSYNC_NORM_PULSE,VI_VSYNC_VALID_NEG_HIGH,

    /*timing信息，对应reg手册的如下配置*/
    /*hsync_hfb    hsync_act    hsync_hhb*/
    {0,            1920,        0,
    /*vsync0_vhb vsync0_act vsync0_hhb*/
     0,            1080,        0,
    /*vsync1_vhb vsync1_act vsync1_hhb*/
     0,            0,            0}
    },
    /*使用内部ISP*/
    VI_PATH_BYPASS,
    /*输入数据类型*/
    VI_DATA_TYPE_RGB
};


typedef enum vi_mode_e
{
    APTINA_AR0130_DC_720P_30FPS = 0,
    APTINA_9M034_DC_720P_30FPS,
    SONY_ICX692_DC_720P_30FPS,
    SONY_IMX104_DC_720P_30FPS,
    SONY_IMX122_DC_1080P_30FPS,
    OMNI_OV9712_DC_720P_30FPS,
    SAMPLE_VI_MODE_1_D1,
}VI_MODE_E;

typedef enum vi_chn_set_e
{
    VI_CHN_SET_NORMAL = 0,
    VI_CHN_SET_MIRROR,
    VI_CHN_SET_FLIP,
    VI_CHN_SET_FLIP_MIRROR
}VI_CHN_SET_E;

typedef struct vi_config_s
{
    VI_MODE_E enViMode;
    VIDEO_NORM_E enNorm;
    ROTATE_E enRotate;
    VI_CHN_SET_E enViChnSet;
}VI_CONFIG_S;

HI_S32 COMM_SYS_Init(VB_CONF_S *pstVbConf);
HI_S32 COMM_VI_StartVi(VI_CONFIG_S* pstViConfig);
HI_S32 COMM_ISP_SensorInit();
HI_S32 COMM_VI_StartDev(VI_DEV ViDev,VI_MODE_E enViMode);
HI_S32 COMM_VI_StartChn(VI_CHN ViChn, RECT_S *pstCapRect, SIZE_S *pstTarSize, VI_CONFIG_S* pstViConfig);

int main()
{
    VB_CONF_S stVbConf;
    HI_S32 s32Ret = HI_SUCCESS;
    VI_CONFIG_S stViConfig;
    VI_DEV_ATTR_S stDevAttr;
    VI_CHN_ATTR_S stChnAttr;
    VIDEO_FRAME_INFO_S FrameInfoA,FrameInfoB;
    VI_EXT_CHN_ATTR_S stExtChnAttr;

    MPP_SYS_CONF_S stSysConf = {0};
    memset(&stVbConf,0,sizeof(VB_CONF_S));
    stVbConf.u32MaxPoolCnt = 128;
    stVbConf.astCommPool[0].u32BlkSize = 1920*1080*2;
    stVbConf.astCommPool[0].u32BlkCnt = 10;

    s32Ret = COMM_SYS_Init(&stVbConf);
    if(HI_SUCCESS != s32Ret)
    {
        printf("system init failed.\n");
    }
    stViConfig.enViMode   = SONY_IMX122_DC_1080P_30FPS;
    stViConfig.enRotate   = ROTATE_NONE;
    stViConfig.enNorm     = VIDEO_ENCODING_MODE_AUTO;
    stViConfig.enViChnSet = VI_CHN_SET_NORMAL;
    s32Ret = COMM_VI_StartVi(&stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        printf("start vi failed!\n");
    }
    stExtChnAttr.enPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    stExtChnAttr.s32BindChn = ViChn;
    stExtChnAttr.stDestSize.u32Width = 320;
    stExtChnAttr.stDestSize.u32Height = 240;
    stExtChnAttr.s32FrameRate = -1;
    stExtChnAttr.s32SrcFrameRate = -1;
    s32Ret = HI_MPI_VI_SetExtChnAttr(ExtChn,&stExtChnAttr);
    if(HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_VI_SetExtChnAttr failed with err code %#x\n",s32Ret);
        return -1;
    }
    s32Ret = HI_MPI_VI_SetFrameDepth(1,5);
    if(HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_VI_SetFrameDepth failed with err code %#x\n",s32Ret);
        return -1;
    }
    s32Ret = HI_MPI_VI_EnableChn(ExtChn);
    if(HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_VI_EnableChn failed with err code %#x\n",s32Ret);
        return -1;
    }

    CvSize img_sz;
    img_sz.width = 320;
    img_sz.height = 240;
    IplImage *eig_image = cvCreateImage(img_sz,IPL_DEPTH_32F,1);
    IplImage *tmp_image = cvCreateImage(img_sz,IPL_DEPTH_32F,1);
    IplImage *imgA = cvCreateImageHeader(img_sz,IPL_DEPTH_8U,1);
    IplImage *imgB = cvCreateImageHeader(img_sz,IPL_DEPTH_8U,1);
    int win_size = 10;
    int corner_count = 500;

    printf("ready to get frame1!\n");
    s32Ret = HI_MPI_VI_GetFrameTimeOut(ExtChn,&FrameInfoA,0);
    if(HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_VI_GetFrameTimeOut faileded with err code %#x!\n",s32Ret);
    }
    printf("get frame1!\n");

    int size = FrameInfoA.stVFrame.u32Stride[0] *FrameInfoA.stVFrame.u32Height;
    char *pImgB =NULL;
    char *pImgA = (char*)HI_MPI_SYS_Mmap(FrameInfoA.stVFrame.u32PhyAddr[0],size);
    cvSetData(imgA,pImgA,FrameInfoA.stVFrame.u32Stride[0]);
    CvPoint2D32f* cornersA = new CvPoint2D32f[500];
//    CvPoint2D32f* cornersB = new CvPoint2D32f[500];

    cvGoodFeaturesToTrack
    (
        imgA,
        eig_image,
        tmp_image,
        cornersA,
        &corner_count,
        0.01,
        5.0,
        0,
        3,
        0,
        0.04
    );
    printf("cvGoodFeaturesToTrack ok!\n");
    int i=5;
    while(i)
    {
        i--;
        s32Ret = HI_MPI_VI_GetFrameTimeOut(ExtChn,&FrameInfoB,0);
        if(HI_SUCCESS != s32Ret)
        {
            printf("HI_MPI_VI_GetFrameTimeOut faileded with err code %#x!\n",s32Ret);
            continue;
        }
        printf("get Frame!\n");

        pImgB = (char *)HI_MPI_SYS_Mmap(FrameInfoB.stVFrame.u32PhyAddr[0],size);
        cvSetData(imgB,pImgB,FrameInfoB.stVFrame.u32Stride[0]);
        char features_found[500];
        float feature_errors[500];

        CvSize pyr_sz = cvSize(imgA->width+8,imgB->height/3);

        IplImage *pyrA = cvCreateImage(pyr_sz, IPL_DEPTH_32F, 1);
        IplImage *pyrB = cvCreateImage(pyr_sz, IPL_DEPTH_32F, 1);
        IplImage *flow = cvCreateImage(img_sz, IPL_DEPTH_32F, 2);

        CvPoint2D32f* cornersB = new CvPoint2D32f[500];
#if 0
        cvCalcOpticalFlowPyrLK
        (
            imgA,
            imgB,
            pyrA,
            pyrB,
            cornersA,
            cornersB,
            corner_count,
            cvSize(320,240),
            5,
            features_found,
            feature_errors,
            cvTermCriteria(CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 20, .3),
            0
        );
#endif
        cvCalcOpticalFlowFarneback(imgA,imgB,flow,0.5,3,15,3,5,1.2,0);
        //do something .....
#if 0
        IplImage *imgC = cvCreateImage(img_sz,IPL_DEPTH_8U,3);
        cvCvtColor(imgB,imgC,CV_GRAY2RGB);
        for(int i=0; i<corner_count;i++)
        {
            CvPoint p0 = cvPoint(cvRound(cornersB[i].x),cvRound(cornersB[i].y));
            cvLine(imgC,p0,p0,CV_RGB(255,0,0),2);
        }
        char str[20]={};
        sprintf(str,"saveImage%d.jpg",5-i);
        cvSaveImage(str,imgC);

        cvReleaseImage(&imgC);
#endif
        cvReleaseImage(&pyrA);
        cvReleaseImage(&pyrB);
        imgA = imgB;
        HI_MPI_VI_ReleaseFrame(ExtChn, &FrameInfoA);
        memset(&FrameInfoA,0,sizeof(FrameInfoA));
        memcpy(&FrameInfoA,&FrameInfoB,sizeof(FrameInfoB));
        memset(&FrameInfoB,0,sizeof(FrameInfoB));
        delete[] cornersA;
        cornersA = cornersB;
    }
    return 0;
}


HI_S32 COMM_SYS_Init(VB_CONF_S *pstVbConf)
{
    MPP_SYS_CONF_S stSysConf ={0};
    HI_S32 s32Ret = HI_FAILURE;
    HI_MPI_SYS_Exit();
    HI_MPI_VB_Exit();

    if(NULL == pstVbConf)
    {
        printf("input parameter is null,it is invaild!\n");
        return HI_FAILURE;
    }
    s32Ret = HI_MPI_VB_SetConf(pstVbConf);
    if(HI_SUCCESS!=s32Ret)
    {
        printf("HI_MPI_VB_SetConf failed! with err code %#x\n",s32Ret);
        return HI_FAILURE;
    }
    s32Ret = HI_MPI_VB_Init();
    if(HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_VB_Init failed with err code %#x\n",s32Ret);
        return HI_FAILURE;
    }
    stSysConf.u32AlignWidth = 64;
    s32Ret = HI_MPI_SYS_SetConf(&stSysConf);
    if(HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_MPP_SYS_SetConf failed with err code %#x\n",s32Ret);
        return HI_FAILURE;
    }
    s32Ret = HI_MPI_SYS_Init();
    if(HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_SYS_Init failed with err code %#x\n",s32Ret);
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}

HI_S32 COMM_VI_StartVi(VI_CONFIG_S* pstViConfig)
{
    HI_S32 i,s32Ret = HI_SUCCESS;
    SIZE_S stTargetSize;
    RECT_S stCapRect;
    VI_MODE_E enViMode;
    ROTATE_E enRotate;
    VI_CHN_SET_E enViChnSet;

    if(!pstViConfig)
    {
        printf("%s: null ptr\n",__FUNCTION__);
        return HI_FAILURE;
    }
    enViMode = pstViConfig->enViMode;
    enViChnSet = pstViConfig->enViChnSet;
    enRotate = pstViConfig->enRotate;
#if 0
    s32Ret = COMM_ISP_SensorInit();
    if(HI_SUCCESS != s32Ret)
    {
        printf("Sensor init failed!\n");
        return HI_FAILURE;
    }
#endif
    s32Ret = COMM_VI_StartDev(ViDev, enViMode);
    if (HI_SUCCESS != s32Ret)
    {
        printf("%s: start vi dev[%d] failed!\n", __FUNCTION__, i);
        return HI_FAILURE;
    }

    stCapRect.s32X = 0;
    stCapRect.s32Y = 0;
    switch (enViMode)
    {
        case APTINA_9M034_DC_720P_30FPS:
        case APTINA_AR0130_DC_720P_30FPS:
        case SONY_ICX692_DC_720P_30FPS:
        case SONY_IMX104_DC_720P_30FPS:
        case OMNI_OV9712_DC_720P_30FPS:
            stCapRect.u32Width = 1280;
            stCapRect.u32Height = 720;
            break;

        case SONY_IMX122_DC_1080P_30FPS:
            stCapRect.u32Width = 1920;
            stCapRect.u32Height = 1080;
            break;

        default:
            stCapRect.u32Width = 1280;
            stCapRect.u32Height = 720;
            break;
    }

    stTargetSize.u32Width = 1920;
    stTargetSize.u32Height = 1080;
    s32Ret = COMM_VI_StartChn(ViChn, &stCapRect, &stTargetSize, pstViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        return HI_FAILURE;
    }
    return s32Ret;
}

HI_S32 COMM_ISP_SensorInit()
{
     HI_S32 s32Ret;
     sensor_init();
     sensor_mode_set(0);
     s32Ret = sensor_register_callback();
     if(s32Ret != HI_SUCCESS)
     {
        printf("sensor_register_callback failed.\n");
        return s32Ret;
     }
     return HI_SUCCESS;
}

HI_S32 COMM_VI_StartDev(VI_DEV ViDev,VI_MODE_E enViMode)
{
    HI_S32 s32Ret;
    VI_DEV_ATTR_S    stViDevAttr;
    memset(&stViDevAttr,0,sizeof(stViDevAttr));
    switch (enViMode)
    {
        // case SAMPLE_VI_MODE_1_D1:
        //     memcpy(&stViDevAttr,&DEV_ATTR_BT656D1_1MUX,sizeof(stViDevAttr));
        //     break;

        // case APTINA_AR0130_DC_720P_30FPS:
        // case SONY_ICX692_DC_720P_30FPS:
        // case SONY_IMX104_DC_720P_30FPS:
        // case APTINA_9M034_DC_720P_30FPS:
        //     memcpy(&stViDevAttr,&DEV_ATTR_AR0130_DC_720P,sizeof(stViDevAttr));
        //     break;

        // case OMNI_OV9712_DC_720P_30FPS:
        //     memcpy(&stViDevAttr,&DEV_ATTR_OV9712_DC_720P,sizeof(stViDevAttr));
        //     break;

        case SONY_IMX122_DC_1080P_30FPS:
            memcpy(&stViDevAttr,&DEV_ATTR_IMX122_DC_1080P,sizeof(stViDevAttr));
            break;

        default:
            // memcpy(&stViDevAttr,&DEV_ATTR_AR0130_DC_720P,sizeof(stViDevAttr));
            memcpy(&stViDevAttr,&DEV_ATTR_IMX122_DC_1080P,sizeof(stViDevAttr));
    }

    s32Ret = HI_MPI_VI_SetDevAttr(ViDev, &stViDevAttr);
    if (s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_VI_SetDevAttr failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_VI_EnableDev(ViDev);
    if (s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_VI_EnableDev failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;

}


HI_S32 COMM_VI_StartChn(VI_CHN ViChn, RECT_S *pstCapRect, SIZE_S *pstTarSize, VI_CONFIG_S* pstViConfig)
{
    HI_S32 s32Ret;
    VI_CHN_ATTR_S stChnAttr;
    ROTATE_E enRotate = ROTATE_NONE;
    VI_CHN_SET_E enViChnSet = VI_CHN_SET_NORMAL;

    if(pstViConfig)
    {
        enViChnSet = pstViConfig->enViChnSet;
        enRotate = pstViConfig->enRotate;
    }

    /* step  5: config & start vicap dev */
    memcpy(&stChnAttr.stCapRect, pstCapRect, sizeof(RECT_S));
    stChnAttr.enCapSel = VI_CAPSEL_BOTH;
    /* to show scale. this is a sample only, we want to show dist_size = D1 only */
    stChnAttr.stDestSize.u32Width = pstTarSize->u32Width;
    stChnAttr.stDestSize.u32Height = pstTarSize->u32Height;
    stChnAttr.enPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;   /* sp420 or sp422 */

    stChnAttr.bMirror = HI_FALSE;
    stChnAttr.bFlip = HI_FALSE;

    switch(enViChnSet)
    {
        case VI_CHN_SET_MIRROR:
            stChnAttr.bMirror = HI_TRUE;
            break;

        case VI_CHN_SET_FLIP:
            stChnAttr.bFlip = HI_TRUE;
            break;

        case VI_CHN_SET_FLIP_MIRROR:
            stChnAttr.bMirror = HI_TRUE;
            stChnAttr.bFlip = HI_TRUE;
            break;

        default:
            break;
    }

    stChnAttr.bChromaResample = HI_FALSE;
    stChnAttr.s32SrcFrameRate = 30;
    stChnAttr.s32FrameRate = 30;

    s32Ret = HI_MPI_VI_SetChnAttr(ViChn, &stChnAttr);
    if (s32Ret != HI_SUCCESS)
    {
        printf("failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    if(ROTATE_NONE != enRotate)
    {
        s32Ret = HI_MPI_VI_SetRotate(ViChn, enRotate);
        if (s32Ret != HI_SUCCESS)
        {
            printf("HI_MPI_VI_SetRotate failed with %#x!\n", s32Ret);
            return HI_FAILURE;
        }
    }

    s32Ret = HI_MPI_VI_EnableChn(ViChn);
    if (s32Ret != HI_SUCCESS)
    {
        printf("failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}
