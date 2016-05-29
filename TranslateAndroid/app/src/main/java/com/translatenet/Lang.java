package com.translatenet;
import java.util.ArrayList;
import java.util.HashMap;
import com.translatenet.Parser;

public class Lang {
	boolean isGroup;
	Lang group;
	String name;
	String code;
	String iso3;
	String country;
	String nativeName;
	HashMap<String, String> dict;
	int avg_price;
	public static ArrayList<Lang> Langs = new ArrayList<Lang>();
	public static final String UNKNOWN = "--";
	public static final String DEFAULT = "en";
    public static final String DEFAULT2 = "fr";
	public static boolean isLang(String code) {
		if (code == null)
			return false;
		for (int i=0; i<Langs.size(); i++) {
			Lang l = Langs.get(i);
			if (!l.isGroup && l.code.compareToIgnoreCase(code) == 0)
				return true;
		}
		return false;
	}
	public static String CodeToLang(String code) {
		if (code == null)
			return "";
		for (int i=0; i<Langs.size(); i++) {
			Lang l = Langs.get(i);
			if (l.code.equalsIgnoreCase(code)) {
				String name = null;
/*				if (l.dict != null) {
					name = l.dict.get(MainActivity.getCurrentLanguage());
					if (name == null)
						name = l.dict.get(Lang.DEFAULT);
				}
*/				if (name == null)
					name = l.name;
				return name; 
			}
		}
		return "";
	}
	public static String CodeToNative(String code) {
		if (code == null)
			return "";
		for (int i=0; i<Langs.size(); i++) {
			Lang l = Langs.get(i);
			if (l.code.equalsIgnoreCase(code))
				return l.nativeName; 
		}
		return "";
	}
	public static String LangToCode(String name) {
		if (name == null)
			return "";
		for (int i=0; i<Langs.size(); i++) {
			Lang l = Langs.get(i);
			if (l.name.equalsIgnoreCase(name))
				return l.code; 
		}
		return "";
	}
	public static String NativeToCode(String name) {
		if (name == null)
			return "";
		for (int i=0; i<Langs.size(); i++) {
			Lang l = Langs.get(i);
			if (l.nativeName.equalsIgnoreCase(name))
				return l.code; 
		}
		return "";
	}
	public static Lang getLangByCode(String code) {
		if (code == null)
			return null;
		for (int i=0; i<Langs.size(); i++) {
			Lang l = Langs.get(i);
			if (l.code.equalsIgnoreCase(code))
				return l;
		}
		return null;
	}
	public static Lang getLangByName(String name) {
		if (name == null)
			return null;
		for (int i=0; i<Langs.size(); i++) {
			Lang l = Langs.get(i);
			if (l.name.equalsIgnoreCase(name))
				return l;
		}
		return null;
	}
	public static boolean isGroup(String name) {
		if (name == null)
			return false;
		for (int i=0; i<Langs.size(); i++) {
			Lang l = Langs.get(i);
			if (l.name.equalsIgnoreCase(name))
				return l.isGroup; 
		}
		return false;		
	}
	public static String parseLocale(String locale) {
		if (locale == null)
			return Lang.DEFAULT;
		if (Lang.isLang(locale))
			return locale;
		String lang;
		int pos = locale.indexOf("-");
		if (pos >= 0)
			lang = locale.substring(0, pos);
		else
			lang = locale;
		if (Lang.isLang(lang))
			return lang;
		else
			return Lang.DEFAULT;
	}
	public static int getAvgPrice(String code) {
		if (code == null)
			return 0;
		Lang l = getLangByCode(code);
		if (l == null)
			return 0;
		return l.avg_price;
	}
}
