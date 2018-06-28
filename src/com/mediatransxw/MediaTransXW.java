package com.mediatransxw;

import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.HashMap;

import android.R.bool;
import android.util.Log;

import com.mediatransxw.MtxwCbInterface;

/**
* 该类提供了调用libmediatransxw。so的方法
* 该库负责传输和编解码，向上层提供媒体参数设置接口、媒体数据发送接口、接收数据递交给上层、数据统计。
* @author wenlinquan
* @Time 2017-3-7 
*/
public class  MediaTransXW{
	
	private static String TAG = "MediaTransXW";
	
	/** 单例模式下，用于内部生成对象。暂不使用 */
	public static MediaTransXW mediatransxw;
	
	/** 回调接口，APK通过mtwxSetCallbackItf注册函数把回调函数传进来。*/
    private static HashMap<Integer, MtxwCbInterface> mCallbackMap = new HashMap<Integer,MtxwCbInterface>();
    
    /*JNI->java 数据接收Buffer*/
    private static HashMap<Integer, ByteBuffer> mRecvDataBuffMap = new HashMap<Integer,ByteBuffer>();
    
    
    public static boolean bSpeechLoad = false;
	
	static{
		try{
			System.loadLibrary("SpeechEngineEnhance");
			bSpeechLoad = true;
			Log.e("MediaTransXW", "Load SpeechEngineEnhance Success!");
		}
		catch (Throwable e) {
			Log.e("MediaTransXW","Can not load SpeechEngineEnhance");  
			e.printStackTrace();
			
		}
		try{
			System.loadLibrary("xwffmpeg");
			Log.e("MediaTransXW", "Load xwffmpeg Success!");
		}
		catch (Throwable e) {
			Log.e("MediaTransXW","Can not load xwffmpeg");  
			e.printStackTrace();
			
		}
		try{
			if(bSpeechLoad)
			{
			    System.loadLibrary("mediatransxw");	
				mtxwNtvActiveSpeechLib(1);
				Log.e("MediaTransXW", "Load mediatransxw Success!");
			}
			else
			{
				System.loadLibrary("mediatransxw_nase");	
				mtxwNtvActiveSpeechLib(0);
				Log.e("MediaTransXW", "Load mediatransxw_nase Success!");
			}
			
		}
		catch (Throwable e) {
			Log.e("MediaTransXW","Can not load libmediatransxw");  
			e.printStackTrace();
		}
	}
	
	/**
	* 不能在外部来创建类对象，而是通过一个方法在类本身来创建	
	*/
	private MediaTransXW(){
		
	}
	
	/*暂不需要提供单例模式
	public static MediaTransXW getInstance() {//这样就可以保证整个程序最多只能有一个cat对象
        if(mediatransxw == null) {  //如果cat为null，就创建cat对象，否则不创建，直接在下面返回已经存在的cat
        	mediatransxw = new MediaTransXW();
        }
        return mediatransxw;
    }
    */

	/**
	* 创建一个媒体传输实例
	* @param type int type-0表示语音，type-1表示视频
	* @return 返回实例id，如果id>0有效
	* @author wenlinquan
	* @Time 2017-3-7 
	*/
	public static int mtxwCreateInstance(int type,int callId){
		Log.i(TAG,"mtxwCreateInstance() type = "+type+" (0-audio 1-video) "+"callId="+callId);
		int instId = mtxwNtvCreateInstance(type,callId);
		if(instId>-1){
			int size = (type==0) ? 320 : (1024*1024*3);
		    mRecvDataBuffMap.put(instId, ByteBuffer.allocateDirect(size));
		    int rslt = mtxwNtvSetRcvBuffer(instId,mRecvDataBuffMap.get(instId),size);
		    Log.e(TAG,"mtxwCreateInstance() call mtxwNtvSetRcvBuffer rslt = "+rslt);
		}
		return instId;
	}
	
	/**
	* 释放媒体传输实例，
	* @param instanceId int  表示要释放的实例id
	* @author wenlinquan
	* @Time 2017-3-7 
	*/
    public static void mtxwDestroyInstance(int instanceId) {
    	Log.i(TAG, "mtxwDestroyInstance()  instanceId="+instanceId);
    	mCallbackMap.remove(instanceId);
    	
    	mtxwNtvDestroyInstance(instanceId);
    	
    	ByteBuffer bb = mRecvDataBuffMap.get(instanceId);
    	mRecvDataBuffMap.remove(instanceId);
   	
	}  
    
	/**
	* 开始运行
	* @param instanceId int  指定要启动运行的实例id
	* @return 返回值-0表示成功
	* @author wenlinquan
	* @Time 2017-3-7 
	*/
    public static int mtxwStart(int instanceId){
    	Log.i(TAG,"mtxwStart() instanceId = "+instanceId);
    	return mtxwNtvStart(instanceId);
    }
    
	/**
	* 停止运行
	* @param instanceId int  指定要停止运行的实例id
	* @return 返回值-0表示成功
	* @author wenlinquan
	* @Time 2017-3-7 
	*/
    public static int mtxwStop(int instanceId){
    	Log.i(TAG,"mtxwStop() instanceId = "+instanceId);
    	return mtxwNtvStop(instanceId);
    }
    
	/**
	* 设置本端地址；返回值：0表示设置成功
	* @param instanceId int  指定的实例id
	* @param ip String 需要设置的本地ip
	* @param port int 需要设置的本地端口
	* @return 返回值-0表示设置成功
	* @author wenlinquan
	* @Time 2017-3-7 
	*/
    public static int mtxwSetLocalAddress(int instanceId,String ip,int port){
    	return mtxwNtvSetLocalAddress(instanceId, ip, port);
    }
    
	/**
	* 设置远端地址；返回值：0表示设置成功
	* @param instanceId int  指定的实例id
	* @param ip String 需要设置的远端ip
	* @param port int 需要设置的远端端口
	* @return 返回值-0表示设置成功
	* @author wenlinquan
	* @Time 2017-3-7 
	*/
    public static int mtxwSetRemoteAddress(int instanceId,String ip,int port){
    	return mtxwNtvSetRemoteAddress(instanceId, ip, port);
    }
	/**
	* 指定网卡；返回值：0表示设置成功
	* @param instanceId int  指定的实例id
	* @param strInterface String 需要指定的接口名称
	* @return 返回值-0表示设置成功
	* @author wenlinquan
	* @Time 2017-3-7 
	*/
    public static int mtxwSetInterface(int instanceId,String strInterface){
    	return mtxwNtvSetInterface(instanceId, strInterface);
    }
    /**
	* 设置Fec参数；返回值：0表示设置成功
	* @param instanceId int  指定的实例id
	* @param InputRank  设置的冗余包阶数(取0-3)
	* @param InputGDLI设置的编码内冗余包个数(0-8,1-16,2-32,3-64个)
	* @param FecSwitch  Fec开关 0-关闭 1-开启
	* @param SimulationDropRate 测试用模拟丢包率取整数值
	* @adaptiveSwitch 自适应算法开关
	* @return 返回值-0表示设置成功
	* @author liuyan
	* @Time 2018-3-28 
	*/
    public static int mtxwSetFecParam(int instanceId,int FecSwitch,int InputGDLI,int InputRank,int SimulationDropRate,int adaptiveSwitch){
    	return mtxwNtvSetFecParam(instanceId,FecSwitch,InputGDLI,InputRank,SimulationDropRate,adaptiveSwitch);
    }
	/**
	* 设置媒体方向
	* @param instanceId int  指定的实例id
	* @param direction int 媒体方向（0-接收和发送，1-仅发送，2-仅接收);
	* @return 返回值-0表示设置成功
	* @author wenlinquan
	* @Time 2017-3-7 
	*/
    public static int mtxwSetDirection(int instanceId,int direction){
    	Log.i("MediaTransXW", "instanceId:"+instanceId+" direction"+direction);
    	return mtxwNtvSetDirection(instanceId, direction);
    }
    
	/**
	* 设置音频编码格式
	* @param instanceId int  指定的实例id
	* @param type int 0-amr,1-G729；返回值：0表示设置成功
	* @return 返回值-0表示设置成功
	* @author wenlinquan
	* @Time 2017-3-7 
	*/
    public static int mtxwSetAudioCodecType(int instanceId,int type){
    	return mtxwNtvSetAudioCodecType(instanceId, type);
    }
    
	/**
	* 设置音频的采样率
	* @param instanceId int  指定的实例id
	* @param sampleRate int 采样率 一般设置为8000
	* @return 返回值-0表示设置成功
	* @author wenlinquan
	* @Time 2017-3-7 
	*/
    public static int mtxwSetAudioSampleRate(int instanceId,int sampleRate){
    	return mtxwNtvSetAudioSampleRate(instanceId, sampleRate);
    }
    
	/**
	* 设置G729参数
	* @param instanceId int  指定的实例id
	* @param frameTime int 帧间隔 例如20ms
	* @return 返回值-0表示设置成功
	* @author wenlinquan
	* @Time 2017-3-7 
	*/
    public static int mtxwSetG729Param(int instanceId,int frameTime){
    	return mtxwNtvSetG729Param(instanceId, frameTime);
    }
    
	/**
	* 设置AMR参数
	* @param instanceId int  指定的实例id
	* @param bitRate int 比特率 单位bit/s(取值4750、12200等)
	* @param frameTime int 帧间隔(取值20 ms）
	* @param payloadFormat int 对齐格式：0-字节对齐，1-节省带宽
	* @return 返回值-0表示设置成功
	* @author wenlinquan
	* @Time 2017-3-7 
	*/
    public static int mtxwSetAmrParam(int instanceId,int bitRate,int frameTime,int payloadFormat){
    	return mtxwNtvSetAmrParam(instanceId, bitRate, frameTime, payloadFormat);
    }
    
	/**
	* 设置视频编码格式
	* @param instanceId int  指定的实例id
	* @param type int 0-H264
	* @return 返回值-0表示设置成功
	* @author wenlinquan
	* @Time 2017-3-7 
	*/
    public static int mtxwSetVideoCodecType(int instanceId,int type){
    	return mtxwNtvSetVideoCodecType(instanceId, type);
    }
    
	/**
	* 设置H264参数
	* @param instanceId int  指定的实例id
	* @param frameRate int 帧率，正确的取值范围[10-30]
	* @param rtpRcvBufferSize int 接收缓存大小，单位为秒，合理取值范围[1-4]
	* @param frameRcvBufferSize int 帧接收缓存大小，单位为秒，合理取值范围[1-8]
	* @param initPlayFactor int 初始播放因子,合理取值范围[0-100]表示 0%-100%
	* @param dicardflag int 烂帧丢弃标志 0：不丢烂帧 1：丢弃烂帧 
	* @return 返回值-0表示设置成功
	* @author wenlinquan
	* @Time 2017-3-7 
	*/
    public static int mtxwSetH264param(int instanceId,int frameRate,int rtpRcvBufferSize,int frameRcvBufferSize,int initPlayFactor,int dicardflag,int playYuvFlag){
    	return mtxwNtvSetH264param(instanceId, frameRate,rtpRcvBufferSize,frameRcvBufferSize,initPlayFactor,dicardflag,playYuvFlag);
    }

	/**
	* 发送数据
	* @param instanceId int  指定的实例id
	* @param data byte[] pcm数据
	* @param uLength int 数据长度
	* @return 返回值-0表示设置成功
	* @author wenlinquan
	* @Time 2017-3-7 
	*/
    public static int mtxwSendAudioData(int instanceId,byte data[],int uLength)
    {
    	return mtxwNtvSendAudioData(instanceId, data, uLength);
    }
    
	/**
	* 发送数据
	* @param instanceId int  指定的实例id
	* @param data byte[] H264frame数据
	* @param uLength int 数据长度
	* @param rotatedAngle int 旋转角度
	* @return 返回值-0表示设置成功
	* @author wenlinquan
	* @Time 2017-3-7 
	*/
    public static int mtxwSendVideoData(int instanceId,byte data[],int uLength,int rotatedAngle)
    {
    	return mtxwNtvSendVideoData(instanceId, data, uLength,rotatedAngle);
    }
    
	/**
	* 设置日志级别
	* @param level int 5:info 4:debug 3:warning 2:erro 1:fatal 0:关闭日志  
	* @return 返回值-0表示设置成功
	* @author wenlinquan
	* @Time 2017-3-7 
	*/
    public static int mtxwSetLogLevel(int level){
    	return mtxwNtvSetLogLevel(level);
    }
    
    /**
	* 获取实例是否运行
	* @param instanceId int 实例id 
	* @return 返回值: 1:表示实例正在运行 0:表示实例未运行   -1:实例不存在  其他值：reserved
	* @author wenlinquan
	* @Time 2017-3-7 
	*/
    public static int mtxwGetInstanceState(int instanceId){
    	return mtxwNtvGetInstanceState(instanceId);
    }
    
	/**
	* 设置语音增强库的时延时间
	* @param time int  时延时间，单位 ms  
	* @return 返回值-0表示设置成功
	* @author wenlinquan
	* @Time 2017-3-20 
	*/
    public static void mtxwSetSpeechEnLibDelayTime(int time){
    	mtxwNtvSetSpeechEnLibDelayTime(time);
    }
    
	/**
	* 设置语音增强库使能去使能
	* @param isEnable int  0：去使能 1：使能  
	* @author wenlinquan
	* @Time 2017-3-20 
	*/
    public static void mtxwActiveSpeechLib(int isEnable){
    	mtxwNtvActiveSpeechLib(isEnable);
    }
    
	/**
	* 获取版本名
	* @return 返回值-String 版本名
	* @author wenlinquan
	* @Time 2017-3-7 
	*/
    public static String mtxwGetVersionName(){
    		return mtxwNtvGetVersionName();
    }
	/**
	*  设置回调接口
	* @param instanceId int  指定的实例id
	* @param cbItfObj MtxwCbInterface MtxwCbInterface是接口类型，定义了相关的回调接口，APK要实现这些接口
	* @return 返回值-0表示设置成功
	* @author wenlinquan
	* @Time 2017-3-7 
	*/   
    public static int mtxwSetCallbackItf(int instanceId,MtxwCbInterface cbItfObj){
    	WeakReference<MtxwCbInterface> wefCBItObj = new WeakReference<MtxwCbInterface>(cbItfObj);
    	//mCallbackMap.put(new Integer(instanceId), cbItfObj);
    	Log.i(TAG, "mtxwSetCallbackItf() instanceId:"+instanceId+" MtxwCbInterface:"+cbItfObj.toString());
    	mCallbackMap.put(instanceId, wefCBItObj.get());
    	return 0;
    }
    
    /**
	*  C库的音频回调函数，把数据送入相应instanceId的RecievePcmData函数
	* @param instanceId int  指定的实例id
	* @param data byte[] c库返回的音频数据
	* @return 返回值-0表示设置成功
	* @author wenlinquan
	* @Time 2017-3-7 
	*/   
    public static int playaudio(int instanceId, byte[] data,int len)
    {
    	MtxwCbInterface cbItfObj ; 
    	cbItfObj = mCallbackMap.get(instanceId);
    	if(null == cbItfObj){
    		Log.e(TAG, "playaudio() cbItfObj is null, instanceId= "+instanceId);
    		return 0;
    	}
    	
    	if(data!=null && data.length == len){
//	    	try {
//	    		cbItfObj.RecievePcmData(instanceId, data);
//			} catch (Exception e) {
//				// TODO: handle exception
//				Log.e(TAG, "playaudio() instanceId"+instanceId+" cbItfObj.RecievePcmData erro:"+e.toString());
//			}
    	}else{
    		if(len>0 && mRecvDataBuffMap.get(instanceId)!=null){
    			//Log.e(TAG,"playaudio() instanceId = " + instanceId + " len = "+len);
	    		//byte[] tmpData = new byte[len];
	    		
	    		try {
	    			//mRecvDataBuffMap.get(instanceId).get(tmpData, 0, len);
		    		cbItfObj.RecievePcmData(instanceId, mRecvDataBuffMap.get(instanceId),len);
		    		mRecvDataBuffMap.get(instanceId).clear();
				} catch (Exception e) {
					// TODO: handle exception
					Log.e(TAG, "playaudio() instanceId 2"+instanceId+" cbItfObj.RecievePcmData erro:"+e.toString());
				}
    		}
    		
    	}
    	return 0;
    }
    
    public static int playvedio(int instanceId, byte[] data,int len,int rotatedAngle,int uWidth,int uHeight, int uSsrc)
    {
    	MtxwCbInterface cbItfObj;
    	//--1--根据instance id find callback obj
    	cbItfObj = mCallbackMap.get(instanceId);
    	if(null == cbItfObj){
    		Log.e(TAG, "playvedio() cbItfObj is null, instanceId="+instanceId);
    		return 0;
    	}
    	if(data!=null && data.length == len){
//	    	try {
//				cbItfObj.RecieveH264FrameData(instanceId, data, rotatedAngle,
//						uWidth, uHeight, uSsrc);
//			} catch (Exception e) {
//				// TODO: handle exception
//				Log.e("mediatrans", "cbItfObj.RecieveH264FrameData erro:"+e.toString());
//			}
    	}else{
    		if(len>0 && mRecvDataBuffMap.get(instanceId)!=null){
	    		//byte[] tmpData = new byte[len];
	    		
	    		try {
	    			//mRecvDataBuffMap.get(instanceId).get(tmpData, 0, len);
					cbItfObj.RecieveH264FrameData(instanceId, mRecvDataBuffMap.get(instanceId),len, rotatedAngle,
							uWidth, uHeight, uSsrc);
					mRecvDataBuffMap.get(instanceId).clear();
				} catch (Exception e) {
					// TODO: handle exception
					Log.e("mediatrans 2 ", "cbItfObj.RecieveH264FrameData erro:"+e.toString());
				}
    		}
    		
    	}
		return 0;
    }
	
    public static int playvedioyuv(int instanceId, byte[] data,int len,int rotatedAngle,int uWidth,int uHeight, int uSsrc)
    {
    	MtxwCbInterface cbItfObj;
    	//--1--根据instance id find callback obj
    	cbItfObj = mCallbackMap.get(instanceId);
    	if(null == cbItfObj){
    		Log.e(TAG, "playvedioyuv() cbItfObj is null instanceId="+instanceId);
    		return 0;
    	}
    	
    	if(data!=null && data.length == len){
//	    	try {
//				cbItfObj.RecieveYUVFrameData(instanceId, data, rotatedAngle,
//						uWidth, uHeight, uSsrc);
//			} catch (Exception e) {
//				// TODO: handle exception
//				Log.e(TAG, "playvedioyuv() instanceId="+instanceId+" cbItfObj.RecieveYUVFrameData erro:"+e.toString());
//			}
    	}else{
    		if(len>0 && mRecvDataBuffMap.get(instanceId)!=null){
	    		try {
					cbItfObj.RecieveYUVFrameData(instanceId, mRecvDataBuffMap.get(instanceId),len, rotatedAngle,
							uWidth, uHeight, uSsrc);
					mRecvDataBuffMap.get(instanceId).clear();
				} catch (Exception e) {
					// TODO: handle exception
					Log.e(TAG, "2 playvedioyuv() instanceId="+instanceId+" cbItfObj.RecieveYUVFrameData erro:"+e.toString());
				}
    		}
    		
    	}
		return 0;
    }
    
    public static int updatestatistic(int instanceId, int statisticId, double statisticValue)
    {
    	MtxwCbInterface cbItfObj;
    	//--1--根据instance id find callback obj
    	cbItfObj = mCallbackMap.get(instanceId);
    	if(null == cbItfObj){
    		Log.e(TAG, "updatestatistic() cbItfObj is null, instanceId="+instanceId);
    		return 0;
    	}
    	try {
			cbItfObj.UpdateStatistic(instanceId, statisticId, statisticValue);
		} catch (Exception e) {
			// TODO: handle exception
			Log.e(TAG, "updatestatistic() instanceId="+instanceId+" cbItfObj.UpdateStatistic erro:"+e.toString());
		}
		return 0;
    }
    
    
    //-----native函数----------------------------------//
    private native static int mtxwNtvCreateInstance(int type,int callId);   
    
    private native static void mtxwNtvDestroyInstance(int instanceId);     
    
    private native static int mtxwNtvStart(int instanceId);    
    
    private native static int mtxwNtvStop(int instanceId);
    
    private native static int mtxwNtvSetLocalAddress(int instanceId,String ip,int port);
    
    private native static int mtxwNtvSetRemoteAddress(int instanceId,String ip,int port);
	
	private native static int mtxwNtvSetInterface(int instanceId,String strInterface);

	private native static int mtxwNtvSetFecParam(int instanceId,int FecSwitch,int InputGDLI,int InputRank,int SimulationDropRate,int adaptiveSwitch);
    
    private native static int mtxwNtvSetDirection(int instanceId,int direction);
    
    private native static int mtxwNtvSetAudioCodecType(int instanceId,int type);
    
    private native static int mtxwNtvSetAudioSampleRate(int instanceId,int sampleRate);
    
    private native static int mtxwNtvSetG729Param(int instanceId,int frameTime);
    
    private native static int mtxwNtvSetAmrParam(int instanceId,int bitRate,int frameTime,int payloadFormat);
    
    private native static int mtxwNtvSetVideoCodecType(int instanceId,int type);
    
    private native static int mtxwNtvSetH264param(int instanceId,int frameRate,int rtpRcvBufferSize,int frameRcvBufferSize,int initPlayFactor,int dicardflag,int playYuvFlag);
    
    private native static int mtxwNtvSendAudioData(int instanceId,byte data[],int uLength);
    
    private native static int mtxwNtvSendVideoData(int instanceId,byte data[],int uLength,int rotatedAngle);
    
    private native static void mtxwNtvSetSpeechEnLibDelayTime(int time);
    
    private native static void mtxwNtvActiveSpeechLib(int isEnable);
    
    private native static int mtxwNtvSetLogLevel(int level);  
    
    private native static int mtxwNtvGetInstanceState(int instanceId); 
    
    private native static String mtxwNtvGetVersionName();
    
    private native static int mtxwNtvSetRcvBuffer(int instanceId,Object buffer,int uLength);
  
}
