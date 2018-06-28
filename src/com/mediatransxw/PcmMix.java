package com.mediatransxw;



public class PcmMix {
	private static final String PCM_MIX = "pcmmix";

    static {
        System.loadLibrary(PCM_MIX);
    }
    
    public static native int Mix(byte pcm1[],int len1,byte pcm2[],int len2,byte outBuff[], int outBuffLen);

}
