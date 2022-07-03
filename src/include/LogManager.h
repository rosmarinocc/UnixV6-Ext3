#ifndef LOG_MANAGER_H
#define LOG_MANAGER_H

#include "Buf.h"
#include "BUfferManager.h"
// need ATABlockDevice.strategy
#include "BlockDevice.h"

/*The Log form is Whole log */
/* The superblock of log*/
class LogSuperBlock
{
public:
  LogSuperBlock();
  ~LogSuperBlock();
  void AddTxB();
  void AddTxE();
  void AddCKPT();
  void InitFlag(); // clear the three plus one

public:
  int TxB;        /*begin*/
  int TxE;        /*end*/
  int CKPT; //0xFFFFFFFF表示打上了标记，0x0表示没有标记
  // int StartAddr; // The addr of the log sector
  int log_pointer; // point to the stack top 
  int LogBufferBlkno[15];  // 要写到磁盘数据区的物理块号  //record the write position of disk -> BitMap
  int LogBufferWcount[15]; //record the Bytes need to transfer
  short LogBufferDev[15]; //record the dev number
  // unsigned char * LogBufferBaddr[15]; //Record the Buf baddr 
  bool padding[512-166];
  // The length is 512 Bytes
};

// LogManager, control the record of system log
class LogManager
{
public:
  static const int StartExist = 0xA;      //    01010
  static const int EndExist = 0x15;       //  10101
  static const int CheckPointExist = 0x9; // 01001

  static const int LOGDEV = 0;
  static const int  BlKNUM = 15;
  static const int  BlKSIZE = 512;
  static const int LOG_SUPER_BLOCK_SECTOR_NUMBER = 184; //start of log sb

  static const int LOG_DATA_Zone_Size = 15; // num of buffer num
  static const int LOG_DATA_START = 185;    // start of data
  static const int LOG_DATA_END = 200 - 1;

public:
  LogSuperBlock *Lspb;

public:
  LogManager();
  ~LogManager();
  void Load_LogSuperBlock();
  void Initialize();
  void LogSpbUpdate();
  void Recover();
  void RedoLog();
  void ReleaseLog();  //clear the superblock area
  // void LogProcessend();
  void WriteLog(Buf *bp); //从缓冲区到日志区
  void WriteLog2Disk();  //从磁盘的日志区到磁盘数据区，判断依据是指针下标
  //void updatebuf(); //除掉log标记
  void logging();
  void IODone(Buf* bp);
  //用于清零读数组
  void resetreadbuf();
public:
  // void LogIOWait(Buf* bp); //需要重新
  // void LogIODone(Buf* bp);

private:
  Buf spbbuf; //当写满时，可能还需要getblk来取，这时候，就陷入了递归
  Buf readbuf[BlKNUM]; //为了不使用Bread，而引发GetBlk,申请readbuf；
  Buf * pointbuf[BlKNUM]; 
  unsigned char Rbuffer[BlKSIZE]; //buf.addr指向的缓冲区
  BufferManager * m_buffer_manager; //需要缓存管理模块(BufferManager)提供的接口
  DeviceManager * m_device_manager; //指向全局device manager
  // SystemManager
};

#endif
