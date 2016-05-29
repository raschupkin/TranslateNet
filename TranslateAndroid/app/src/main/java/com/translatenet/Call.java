package com.translatenet;
import java.util.Date;
import android.text.format.Time;

public class Call {
	boolean active;
	User client;
	User translator;
	int cost;
	String TranslateLang;
	String ClientLang;
	int price;
	Date start = new Date();
	Date end = new Date();
	int length;

    boolean displayed;     // for HistoryFragment

	public void initFromPacket(Parser.PacketPhonecallStatus p, User user) {
		active = p.active;
		if (client != null)
			client.balance = p.balance;
		length = p.time;
		cost = p.cost;
		TranslateLang = p.translate_lang;
		ClientLang = p.client_lang;
		if (user.isT) {
			client.id = p.peer;
			translator.id = user.id;
		} else {
			client.id = user.id;
			translator.id = p.peer;
		}
		if (translator != null)
			translator.name = p.translator_name;
		if (client != null)
			client.name = p.client_name;
		price = p.price;
	}
}
