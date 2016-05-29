package com.translatenet;

public class Baresip {
static {
	System.loadLibrary("Baresip");
}
public native void mod_init();
}
