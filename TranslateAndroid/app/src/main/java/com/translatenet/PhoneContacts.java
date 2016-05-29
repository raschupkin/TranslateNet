package com.translatenet;
import java.util.ArrayList;

import android.content.ContentResolver;
import android.database.Cursor;
import android.provider.ContactsContract;
import android.provider.ContactsContract.PhoneLookup;
import android.telephony.PhoneNumberUtils;

public class PhoneContacts {
	ArrayList<String> contacts = new ArrayList<String>();
	public void LoadContacts(ContentResolver cr) {
		if (cr == null)
			return;
		Cursor cur = cr.query(ContactsContract.CommonDataKinds.Phone.CONTENT_URI, null, null, null, null);
		int idx = cur.getColumnIndex(ContactsContract.CommonDataKinds.Phone.NUMBER);
		if (cur.getCount() > 0)
			while (cur.moveToNext()) {
				String num = cur.getString(idx);
				contacts.add(num);
			}		
	}
boolean DEBUG_TEST_ALERT = false;
	public boolean isPhoneInContact(String phone) {
		if (DEBUG_TEST_ALERT)
			return false;
		for (int i=0; i<contacts.size(); i++)
			if (PhoneNumberUtils.compare(contacts.get(i), phone)) {
				return true;
			}
		return false;
	}
}
