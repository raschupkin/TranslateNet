package com.translatenet;

import java.util.HashMap;
import java.util.ArrayList;

public class UserData {
	boolean isT;
	int id;
	String phone;
	int balance;
	String email;
	String password;
	String name;
	String lang;
	public HashMap<String, Integer> translate = new HashMap<String, Integer>();
	public static ArrayList<String> AllLangs;
	static public void initLangs() {
		AllLangs.add("eng");
		AllLangs.add("fre");
		AllLangs.add("spa");
	}
	static public boolean isLang(String lang) {
		if (AllLangs.contains(lang))
			return true;
		else
			return false;
	}
}
