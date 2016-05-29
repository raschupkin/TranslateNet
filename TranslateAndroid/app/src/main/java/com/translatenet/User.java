package com.translatenet;

import java.util.HashMap;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.Map;

public class User implements Cloneable {
	boolean isT;
	String client_lang, translate_lang;
	int price;
	
	int id;
	public static final int PHONE_STATUS_NONE = 0;
	public static final int PHONE_STATUS_CONFIRMED = 1;
	public static final int PHONE_STATUS_AWAIT = 2;
	int phone_status;
	String phone;
    String await_phone;
	int balance;
	String email;
	String password;
	String name;
	String lang;
	int rating;
	int rating_num;
	String paypal_email;
	public HashMap<String, Integer> translate = new HashMap<String, Integer>();
	String country;
	
	boolean busy;
	boolean delete;

    boolean await, confirmed, rejected, error;       // like in iOS in case activity kill because of memory clean

	static String formatPhone(String phone) {
		StringBuilder newPhone = new StringBuilder();
		for (int i = 0; i < phone.length(); i++)
			if ((phone.charAt(i) >= '0' && phone.charAt(i) <= '9') || (phone.charAt(i) == '+' && i == 0))
				newPhone.append(phone.charAt(i));
		return newPhone.toString();
	}
	
	public String getCommonLang(User c) {
		if (translate.get(c.lang) != null)
			return c.lang;
		return "";
	}
	public boolean CompareTranslate(HashMap<String, Integer> translate2) {
		if (!translate.entrySet().equals(translate2.entrySet()))
			return false;
		for (HashMap.Entry<String , Integer> entry : translate.entrySet())
			if (entry.getValue() != translate2.get(entry.getKey()))
				return false;
		return true;
	}
	protected boolean CheckLangs() {
		if (!isT) {
			if (lang == null)
				return true;
			if (!Lang.isLang(lang))
				return false;
		} else {
			Iterator it = translate.entrySet().iterator();
			boolean removed = false;
			while (it.hasNext()) {
				Map.Entry<String, Integer> t = (Map.Entry<String, Integer>)it.next();
				if (!Lang.isLang(t.getKey())) {
					it.remove();
					removed = true;
				}
			}
			if (removed) {
				return false;
			}
		}
		return true;
	}
	protected Object clone() throws CloneNotSupportedException {
        User u2 = (User)super.clone();
        u2.translate = (HashMap<String, Integer>)translate.clone();
        return u2;
	}
}
