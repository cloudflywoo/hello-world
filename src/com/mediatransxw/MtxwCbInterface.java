package com.mediatransxw;

import java.nio.ByteBuffer;

public interface MtxwCbInterface {
	
	public void RecievePcmData(int instanceId,ByteBuffer data,int len);
	
	public void RecieveH264FrameData(int instanceId,ByteBuffer data,int len,int rotatedAngle,int uWidth,int uHeight, int uSsrc);
	public void RecieveYUVFrameData(int instanceId,ByteBuffer data,int len,int rotatedAngle,int uWidth,int uHeight, int uSsrc);
	
	public void UpdateStatistic(int instanceId, int statisticId, double statisticValue);
	
}
