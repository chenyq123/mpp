#include "../extdrv/tw2865/tw2865.h"
#include "../include/hi_common.h"
#include "../include/hi_comm_sys.h"
#include "../include/hi_comm_vb.h"
#include "../include/hi_comm_isp.h"
#include "../include/hi_comm_vi.h"
#include "../include/hi_defines.h"

#include "../include/mpi_sys.h"
#include "../include/mpi_vb.h"
#include "../include/mpi_vi.h"
#include "../include/mpi_isp.h"

#include <stdio.h>
#include <string.h>


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
    VI_DEV ViDev = 0;
    VI_CHN ViChn = 0;
    VI_DEV_ATTR_S stDevAttr;
    VI_CHN_ATTR_S stChnAttr;

    MPP_SYS_CONF_S stSysConf = {0};
    memset(&stVbConf,0,sizeof(VB_CONF_S));
    stVbConf.u32MaxPoolCnt = 128;
    stVbConf.astCommPool[0].u32BlkSize = 320*240*2;
    stVbConf.astCommPool[0].u32BlkCnt = 6;

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
    VI_DEV ViDev;
    VI_CHN ViChn;
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
    s32Ret = COMM_ISP_SensorInit();
    if(HI_SUCCESS != s32Ret)
    {
        printf("Sensor init failed!\n");
        return HI_FAILURE;
    }
    ViDev = 0;
    s32Ret = COMM_VI_StartDev(ViDev, enViMode);
    if (HI_SUCCESS != s32Ret)
    {
        printf("%s: start vi dev[%d] failed!\n", __FUNCTION__, i);
        return HI_FAILURE;
    }
    ViChn = 0;

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
