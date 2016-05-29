package com.translatenet;

public class Options {
    public static final int PHONE_REGISTER_NONE     = 0;
    public static final int PHONE_REGISTER_SETUP    = 1;
    public static final int PHONE_REGISTER_BALANCE  = 2;
    public static final int PHONE_REGISTER_ANY      = 3;
    int PhoneRegisterMode;
	int FeeMarket;
	int FeeApp;
    int CallTimeFree;
    int CallMinBalance;
    int CallMinTimeRating;
    boolean ActiveTSearch;
}
