#ifndef FIFOBUFFER_H
#define FIFOBUFFER_H
#include <windows.h>

#define  AUDIOBUFLEN (8*65536)
struct AVFrameBuffer 
{
	AVFrameBuffer *prev;
	AVFrameBuffer *next;	
	unsigned char *context;
	int width ;
	int height ;
	int size;
	//Frame bit rate
	int frameLen;
	int frameType;
	int sampleRate;
};

class FifoBuffer 
{
public:
	FifoBuffer();
	~FifoBuffer();

	int init(int count,int chunksize);
	int reset();
	int resize(int chunksize);
	int clear();
	bool write(AVFrameBuffer* vframe) ;
	bool read(AVFrameBuffer* vframe) ;
	int chunkCount();//有效数据 块数
	void setlock(bool iflock) ;
	bool getlock();
	BYTE* GetLastFrame();
	BYTE* GetNextWritePos();

	void SetbReadNull(bool ifReadNull){m_bReadNull = ifReadNull;}
	bool GetIfReadNull(){return m_bReadNull;}

	void GetLastFrameBuf(unsigned char* pBuf){m_dataFrame.context = pBuf;}

private:		
	bool createFreeList();	
	void cleanFreeList();
	void cleanDataList();
	
	AVFrameBuffer *getFreeNode();
	AVFrameBuffer *getDataNode();
	
	void appendToFreeList(AVFrameBuffer *item);
	void appendToDataList(AVFrameBuffer *item);

private:

	unsigned char *m_mempool;//内存池

	bool m_ifLock ;//如果锁定，则write函数中一直等待有空的缓冲块，在定位操作时，将它置为False,以使write函数尽快返回
	bool m_inited; // 初始化成功标志
	int m_count; // 节点总数
	int m_chunksize;
	AVFrameBuffer *m_freeQueueFirst;
	AVFrameBuffer *m_freeQueueLast;
	int m_freeCount;//空的块的数

	AVFrameBuffer *m_outQueueFirst;
	AVFrameBuffer *m_outQueueLast;
	int m_outCount;//有效数据块数

	AVFrameBuffer m_dataFrame;
	CRITICAL_SECTION m_DataAccessLock ;

	bool m_bReadNull;
};

#endif 




















