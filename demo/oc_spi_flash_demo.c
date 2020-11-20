/* Copyright (C) 2018 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * This software is supplied "AS IS" without any warranties.
 * RDA assumes no responsibility or liability for the use of the software,
 * conveys no license or title under any patent, copyright, or mask work
 * right to the product. RDA reserves the right to make changes in the
 * software without notification.  RDA also make no representation or
 * warranty that such application will be suitable for the specified use
 * without further testing or modification.
 */

#define OSI_LOG_TAG OSI_MAKE_LOG_TAG('S', 'P', 'I', 'F')

#include "fibo_opencpu.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"


uint8_t OSI_ALIGNED(16) recvdata[2048] = {0};
uint8_t OSI_ALIGNED(16) sendata[2048] = {0};

#define sFLASH_ID 0xEF4018

#define SPI_FLASH_PageSize 256
#define SPI_FLASH_PerWritePageSize 256

#define W25x_WriteEnable 0x06
#define W25x_writeDisable 0x04
#define W25x_ReadStatusReg 0x05
#define W25x_WriteStatusReg 0x01
#define W25x_ReadData 0x03
#define W25x_FastReadData 0x0B
#define W25x_FastReadDual 0x3B
#define W25x_PageProgram 0x02
#define W25x_BlockErase 0xD8
#define W25x_SectorEarse 0x20
#define W25x_ChipErase 0xC7
#define W25x_PowerDone 0xB9
#define W25x_JedecDeviceID 0x9F


SPIHANDLE spiFd;

void testSpi1Init(void)
{
    hwp_iomux->pad_gpio_8_cfg_reg = 0x01;
    hwp_iomux->pad_gpio_9_cfg_reg = 0x01;
    hwp_iomux->pad_gpio_10_cfg_reg = 0x01;
    hwp_iomux->pad_gpio_11_cfg_reg = 0x01;
    hwp_iomux->pad_gpio_12_cfg_reg = 0x01;
    drvSpiConfig_t cfg = {
        0,
    };
    cfg.name = DRV_NAME_SPI1;
    cfg.inputEn = true;
    cfg.baud = 1000000;
    cfg.cpol = SPI_CPOL_LOW;
    cfg.cpha = SPI_CPHA_1Edge;
    cfg.input_sel = SPI_DI_1;
    cfg.transmode = SPI_DMA_POLLING;
    cfg.cs_polarity0 = SPI_CS_ACTIVE_LOW;
    cfg.framesize = 8;
    memset(recvdata, 0x00, 2048);

	fibo_spi_open(cfg, &spiFd);
    osiThreadSleep(1000);
}

bool WaitWriteFininsh(void)
{
    uint8_t OSI_ALIGNED(16) cmd = W25x_ReadStatusReg;
    uint8_t OSI_ALIGNED(16) status;
    uint8_t OSI_ALIGNED(16) dummydata;
	SPI_IOC spiIoc =
	{
		8,
		1000000,
		SPI_I2C_CS0,
		SPI_CS_ACTIVE_LOW,
	};
    fibo_spi_pinctrl(spiFd, SPI_CS0_CTRL, FORCE_0_CTRL);
    fibo_spi_send(spiFd, spiIoc, &cmd, 1);
    uint32_t startTime = osiUpTimeUS();
    do
    {
        fibo_spi_recv(spiFd, spiIoc, &dummydata, &status, 1);
        if (osiUpTimeUS() - startTime > 2000000)
        {
            return false;
        }
    } while ((status & 0x01) == 1);
    fibo_spi_pinctrl(spiFd, SPI_CS0_CTRL, SPI_CTRL);
    return true;
}

void SpiFlashWriteEnable()
{
    uint8_t OSI_ALIGNED(16) cmd = W25x_WriteEnable;
	SPI_IOC spiIoc =
	{
		8,
		1000000,
		SPI_I2C_CS0,
		SPI_CS_ACTIVE_LOW,
	};
    fibo_spi_send(spiFd, spiIoc, &cmd, 1);
}

void testSpiFlashSectorErase(uint32_t sectoraddr)
{
    uint8_t OSI_ALIGNED(16) cmd[5] = {
        0,
    };
	SPI_IOC spiIoc =
	{
		8,
		1000000,
		SPI_I2C_CS0,
		SPI_CS_ACTIVE_LOW,
	};
    //enable write
    SpiFlashWriteEnable();
    if (!WaitWriteFininsh())
    {
        OSI_LOGI(0, "time out in line %d", __LINE__);
        return;
    }
    cmd[0] = W25x_SectorEarse;
    cmd[1] = (sectoraddr & 0xff0000) >> 16;
    cmd[2] = (sectoraddr & 0xff00) >> 8;
    cmd[3] = sectoraddr & 0xff;
    fibo_spi_pinctrl(spiFd, SPI_CS0_CTRL, FORCE_0_CTRL);
    fibo_spi_send(spiFd, spiIoc, cmd, 4);
    fibo_spi_pinctrl(spiFd, SPI_CS0_CTRL, SPI_CTRL);
    if (!WaitWriteFininsh())
    {
        OSI_LOGI(0, "time out in line %d", __LINE__);
        return;
    }
}

void testSpiPageWrite(uint8_t *pbuff, uint32_t writeaddr, uint16_t size)
{
    uint8_t OSI_ALIGNED(16) cmd[5] = {
        0,
    };
	SPI_IOC spiIoc =
	{
		8,
		1000000,
		SPI_I2C_CS0,
		SPI_CS_ACTIVE_LOW,
	};
    SpiFlashWriteEnable();

    cmd[0] = W25x_PageProgram;
    cmd[1] = (writeaddr & 0xff0000) >> 16;
    cmd[2] = (writeaddr & 0xff00) >> 8;
    cmd[3] = writeaddr & 0xff;
    fibo_spi_pinctrl(spiFd, SPI_CS0_CTRL, FORCE_0_CTRL);
    fibo_spi_send(spiFd, spiIoc, cmd, 4);
    if (size > 256)
    {
        size = 256;
    }
    fibo_spi_send(spiFd, spiIoc, pbuff, size);
    fibo_spi_pinctrl(spiFd, SPI_CS0_CTRL, SPI_CTRL);
    if (!WaitWriteFininsh())
    {
        OSI_LOGI(0, "time out in line %d", __LINE__);
        return;
    }
}

void testSpiFlashWrite(uint8_t *pbuff, uint32_t writeaddr, uint16_t size)
{
    uint8_t addr = writeaddr % 256;
    uint8_t count = 256 - addr;
    uint16_t numofpage = size / 256;
    uint16_t numofsigle = size % 256;

    if (addr == 0)
    {
        if (numofpage == 0)
        {
            testSpiPageWrite(pbuff, writeaddr, size);
        }
        else
        {
            while (numofpage--)
            {
                testSpiPageWrite(pbuff, writeaddr, 256);
                writeaddr += 256;
                pbuff += 256;
            }
            testSpiPageWrite(pbuff, writeaddr, numofsigle);
        }
    }
    else
    {
        if (numofpage == 0)
        {
            if (numofsigle > count)
            {
                uint16_t temp = numofsigle - count;
                testSpiPageWrite(pbuff, writeaddr, count);
                writeaddr += count;
                pbuff += count;
                testSpiPageWrite(pbuff, writeaddr, temp);
            }
            else
            {
                testSpiPageWrite(pbuff, writeaddr, size);
            }
        }
        else
        {
            size -= count;
            numofpage = size / 256;
            numofsigle = size % 256;
            testSpiPageWrite(pbuff, writeaddr, count);
            writeaddr += count;
            pbuff += count;
            while (numofpage--)
            {
                testSpiPageWrite(pbuff, writeaddr, 256);
                writeaddr += 256;
                pbuff += 256;
            }
            if (numofsigle != 0)
            {
                testSpiPageWrite(pbuff, writeaddr, numofsigle);
            }
        }
    }
}

void testSpiFlashRead(uint8_t *pbuff, uint32_t ReadAddr, uint16_t size)
{
    uint8_t OSI_ALIGNED(16) cmd[5] = {
        0,
    };
    uint8_t OSI_ALIGNED(16) dummpdata[size];
	SPI_IOC spiIoc =
	{
		8,
		1000000,
		SPI_I2C_CS0,
		SPI_CS_ACTIVE_LOW,
	};
    cmd[0] = W25x_ReadData;
    cmd[1] = (ReadAddr & 0xff0000) >> 16;
    cmd[2] = (ReadAddr & 0xff00) >> 8;
    cmd[3] = ReadAddr & 0xff;
    fibo_spi_pinctrl(spiFd, SPI_CS0_CTRL, FORCE_0_CTRL);
    fibo_spi_send(spiFd, spiIoc, cmd, 4);
    fibo_spi_recv(spiFd, spiIoc, dummpdata, pbuff, size);
    fibo_spi_pinctrl(spiFd, SPI_CS0_CTRL, SPI_CTRL);
}

void testSpiWrite(void)
{
    uint16_t i;
	SPI_IOC spiIoc =
	{
		8,
		1000000,
		SPI_I2C_CS0,
		SPI_CS_ACTIVE_LOW,
	};
    for (i = 0; i < 10000; i++)
    {
        fibo_spi_send(spiFd, spiIoc, sendata, 10);
        osiThreadSleep(100);
        OSI_LOGI(0, "round %d finish", i);
    }
}

void testSpiRead(void)
{
    uint16_t i;
	SPI_IOC spiIoc =
	{
		8,
		1000000,
		SPI_I2C_CS0,
		SPI_CS_ACTIVE_LOW,
	};
    for (i = 0; i < 10000; i++)
    {
        memset(sendata, 0xaa, 1024);
        fibo_spi_recv(spiFd, spiIoc, sendata, recvdata, 1024);
        osiThreadSleep(100);
        OSI_LOGI(0, "round %d finish", i);
    }
}

void testSpireadwrite(void)
{
    uint16_t i;
    uint16_t j = 0;

    for (i = 0; i < 60000; i++)
    {
        memset(sendata, j, 2048);
        j = j + 1;
        if (j == 256)
            j = 0;
        testSpiFlashSectorErase(0x02000);
        testSpiFlashWrite((uint8_t *)sendata, 0x02000, 2048);
        uint32_t startTime = osiUpTimeUS();
        testSpiFlashRead((uint8_t *)recvdata, 0x02000, 2048);
        uint32_t endtime = osiUpTimeUS() - startTime;
        OSI_LOGI(0, "take  %d ms", endtime / 1000);
        memset(recvdata, 0, 2048);
        osiThreadSleep(2000);
    }
}

uint32_t testSpireadFlashID(void)
{

    uint8_t OSI_ALIGNED(16) Command = W25x_JedecDeviceID;
    uint8_t OSI_ALIGNED(16) FlashId[3] = {0};
    uint8_t OSI_ALIGNED(16) dummydata[3] = {0};
    uint32_t id;
	SPI_IOC spiIoc =
	{
		8,
		1000000,
		SPI_I2C_CS0,
		SPI_CS_ACTIVE_LOW,
	};
    fibo_spi_pinctrl(spiFd, SPI_CS0_CTRL, FORCE_0_CTRL);
    fibo_spi_send(spiFd, spiIoc, &Command, 1);
    fibo_spi_recv(spiFd, spiIoc, dummydata, FlashId, 3);
    fibo_spi_pinctrl(spiFd, SPI_CS0_CTRL, SPI_CTRL);
    OSI_LOGI(0, "read id %02x,%02x,%02x", FlashId[0], FlashId[1], FlashId[2]);
    id = (FlashId[0] << 16) | (FlashId[1] << 8) | FlashId[2];
    return id;
}


static void prvThreadEntry(void *param)
{	
    testSpi1Init();
    testSpireadFlashID();
    testSpireadwrite();
    OSI_LOGD(0, "prvThreadEntry osiThreadExit");
    osiThreadExit();
}




void* appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);

    fibo_thread_create(prvThreadEntry, "mythread", 1024, NULL, OSI_PRIORITY_NORMAL);
    return 0;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}


