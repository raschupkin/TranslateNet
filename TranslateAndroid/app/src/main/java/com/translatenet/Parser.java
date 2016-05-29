package com.translatenet;


import org.xmlpull.v1.XmlPullParser;

import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.StringReader;

import android.util.Xml;

public class Parser {
	public static final int	ERROR_NOERROR  						=	0;
	public static final int ERROR_OTHER							=	1;
	public static final int ERROR_MAINTENANCE                   =   2;
	public static final int ERROR_VERSION						=	3;
	public static final int ERROR_LOAD							=	4;
	public static final int ERROR_FORMAT                        =   5;
	public static final int ERROR_NO_USER                       =   6;
	public static final int ERROR_USER_OFFLINE                  =   7;
	public static final int ERROR_USER_ALREADY_EXIST       		=	8;
    public static final int ERROR_NAME_EXIST                    =   9;
	public static final int ERROR_ALREADY_LOGIN                 =   10;
	public static final int ERROR_LOGIN_REQUIRED            	=	11;
	public static final int ERROR_WRONG_PASSWORD            	=	12;
	public static final int ERROR_WRONG_SMSCODE                 =   13;
	public static final int ERROR_NO_USERDATA                   =   14;
	public static final int ERROR_NO_PHONE                      =   15;
    public static final int ERROR_PHONE_CHANGED                 =   16;
	public static final int ERROR_NO_LANG                       =   17;
	public static final int ERROR_PHONE_AWAITING           		=	18;
	public static final int ERROR_TEMP_BLOCKED     	            =   19;
	public static final int ERROR_UNKOWN_CALL                   =   20;
	public static final int ERROR_CALL_EXIST                    =   21;
	public static final int ERROR_CALL_STATE                    =   22;
	public static final int ERROR_BALANCE                       =   23;
	public static final int ERROR_PHONECALL_ERROR           	=	24;
	public static final int ERROR_PEER_DISCON					=	25;
	public static final int ERROR_RATING_ERROR                  =	26;
	public static final int ERROR_PAYPAL_TRANSFER_ACTIVE	    =	27;
	public static final int ERROR_PURCHASE_SIGNATURE			=	28;
    public static final int ERROR_ANOTHER_LOGIN                 =   29;
    public static final int ERROR_ANOTHER_PHONE                 =   30;
    public static final int ERROR_UNKNOWN_COUNTRY               =   31;

	public String parseType(String packet) {
		String type = "";
		try {
			xpp.setInput(new StringReader(packet));
			int eventType = xpp.getEventType();
			while (eventType != XmlPullParser.END_DOCUMENT) {
				type = xpp.getName();
				if (eventType == XmlPullParser.START_TAG)
					return type;
				eventType = xpp.next();
			}
		} catch (Exception ex) {
			return "";
		}
		return type;
	}
	public class PacketError {
		public int code;
		public String command;
		int phonecall_request_client = 0;
		int phonecall_request_translator = 0;
        public String message;
        int id = 0;
        int sms_sent_num = 0, sms_block_days = 0;
	}
	public PacketError parseError(String packet) {
		PacketError p = new PacketError();
		p.code = -1;
		String type;
		try {
			xpp.setInput(new StringReader(packet));
			int eventType = xpp.getEventType();
			while (eventType != XmlPullParser.END_DOCUMENT) {
				type = xpp.getName();
				if (eventType == XmlPullParser.START_TAG) {
					if (!type.equalsIgnoreCase("error"))
						return null;
					int depth = xpp.getDepth();
					do {
						if (xpp.next() == XmlPullParser.START_TAG) {
							if (xpp.getName().equalsIgnoreCase("code")) {
								if (xpp.next() == XmlPullParser.TEXT)
									p.code = Integer.parseInt(xpp.getText());
							} else if (xpp.getName().equalsIgnoreCase("command")) {
								if (xpp.next() == XmlPullParser.TEXT)
									p.command = xpp.getText();
							} else if (xpp.getName().equalsIgnoreCase("client")) {
								if (xpp.next() == XmlPullParser.TEXT)
									p.phonecall_request_client = Integer.parseInt(xpp.getText());
							} else if (xpp.getName().equalsIgnoreCase("translator")) {
								if (xpp.next() == XmlPullParser.TEXT)
									p.phonecall_request_translator = Integer.parseInt(xpp.getText());
                            } else if (xpp.getName().equalsIgnoreCase("message")) {
                                if (xpp.next() == XmlPullParser.TEXT)
                                    p.message = xpp.getText();
							} else if (xpp.getName().equalsIgnoreCase("id")) {
                                if (xpp.next() == XmlPullParser.TEXT)
                                    p.id = Integer.parseInt(xpp.getText());
                            } else if (xpp.getName().equalsIgnoreCase("sms_sent_num")) {
                                if (xpp.next() == XmlPullParser.TEXT)
                                    p.sms_sent_num = Integer.parseInt(xpp.getText());
                            } else if (xpp.getName().equalsIgnoreCase("sms_block_days")) {
                                if (xpp.next() == XmlPullParser.TEXT)
                                    p.sms_block_days = Integer.parseInt(xpp.getText());
                            } else
								;
						}
					} while (xpp.getDepth() > depth);
					if (p.command == null)
						p.command = "";
				}
				eventType = xpp.next();
			}
		} catch (Exception ex) {
			return null;
		}
		return p;
	}
	class PacketPhonecallStatus {
		int peer;
		int error;
		int cost;
		int balance;
		boolean active;
		int time;
		String translate_lang;
		String client_lang;
		String translator_name;
		String client_name;
		int price;
	}
	public PacketPhonecallStatus parsePhonecallStatus(String packet) {
		PacketPhonecallStatus p = new PacketPhonecallStatus();
		String type;
		try {
			xpp.setInput(new StringReader(packet));
			int eventType = xpp.getEventType();
			boolean exist_peer = false, exist_time = false;
			while (eventType != XmlPullParser.END_DOCUMENT) {
				type = xpp.getName();
				if (eventType == XmlPullParser.START_TAG) {
					if (type.equalsIgnoreCase("peer")) {
						if (xpp.next() == XmlPullParser.TEXT) {
							p.peer = Integer.parseInt(xpp.getText());
							exist_peer = true;
						}
					} else if (type.equalsIgnoreCase("error")) {
						if (xpp.next() == XmlPullParser.TEXT)
							p.error = Integer.parseInt(xpp.getText());
					} else if (type.equalsIgnoreCase("cost")) {
						if (xpp.next() == XmlPullParser.TEXT)
							p.cost = Integer.parseInt(xpp.getText());
					} else if (type.equalsIgnoreCase("balance")) {
						if (xpp.next() == XmlPullParser.TEXT)
							p.balance = Integer.parseInt(xpp.getText());
					} else if (type.equalsIgnoreCase("active")) {
						if (xpp.next() == XmlPullParser.TEXT)
							p.active = Integer.parseInt(xpp.getText()) != 0;
					} else if (type.equalsIgnoreCase("time")) { 
						if (xpp.next() == XmlPullParser.TEXT) {
							p.time = Integer.parseInt(xpp.getText());
							exist_time = true;
						}
					} else if (type.equalsIgnoreCase("translate_lang")) { 
						if (xpp.next() == XmlPullParser.TEXT)
							p.translate_lang = xpp.getText();
					} else if (type.equalsIgnoreCase("client_lang")) { 
						if (xpp.next() == XmlPullParser.TEXT)
							p.client_lang = xpp.getText();
					} else if (type.equalsIgnoreCase("translator_name")) { 
						if (xpp.next() == XmlPullParser.TEXT)
							p.translator_name = xpp.getText();
					} else if (type.equalsIgnoreCase("client_name")) { 
						if (xpp.next() == XmlPullParser.TEXT)
							p.client_name = xpp.getText();
					} else if (type.equalsIgnoreCase("price")) {
						if (xpp.next() == XmlPullParser.TEXT)
							p.price = Integer.parseInt(xpp.getText());
					} else
						;
				}
				eventType = xpp.next();
			}
//			if (!exist_peer || !exist_time)
//				return null;
		} catch (Exception ex) {
			return null;
		}
		return p;
	}
	class PacketPhonecallRequest {
		int client;
		String name;
		String translate_lang;
		String client_lang;
		int price;
		String country;
		int balance;
	}
	public PacketPhonecallRequest parsePhonecallRequest(String packet) {
		PacketPhonecallRequest p = new PacketPhonecallRequest();
		String type;
		try {
			xpp.setInput(new StringReader(packet));
			int eventType = xpp.getEventType();
			while (eventType != XmlPullParser.END_DOCUMENT) {
				type = xpp.getName();
				if (eventType == XmlPullParser.START_TAG) {
					if (type.equalsIgnoreCase("client")) {
						if (xpp.next() == XmlPullParser.TEXT)
							p.client = Integer.parseInt(xpp.getText());
					} else if (type.equalsIgnoreCase("name")) {
						if (xpp.next() == XmlPullParser.TEXT)
							p.name = xpp.getText();
					} else if (type.equalsIgnoreCase("translate_lang")) {
						if (xpp.next() == XmlPullParser.TEXT)
							p.translate_lang = xpp.getText();
					} else if (type.equalsIgnoreCase("client_lang")) {
						if (xpp.next() == XmlPullParser.TEXT)
							p.client_lang = xpp.getText();
					} else if (type.equalsIgnoreCase("price")) {
						if (xpp.next() == XmlPullParser.TEXT)
							p.price = Integer.parseInt(xpp.getText());
					} else if (type.equalsIgnoreCase("country")) {
						if (xpp.next() == XmlPullParser.TEXT)
							p.country = xpp.getText();
					} else if (type.equalsIgnoreCase("balance")) {
						if (xpp.next() == XmlPullParser.TEXT)
							p.balance = Integer.parseInt(xpp.getText());
					} else
						;
				}
				eventType = xpp.next();
			}
			if (p.client <= 0 || p.name == null || p.price <= 0 || p.balance <= 0)
				return null;
		} catch (Exception ex) {
			return null;
		}
		return p;
	}
	class PacketPhonecallConfirm {
		int translator;
		boolean accept;
		String phone;
	}
	public PacketPhonecallConfirm parsePhonecallConfirm(String packet) {
		PacketPhonecallConfirm p = new PacketPhonecallConfirm();
		String type;
		boolean accept_exist = false;
		try {
			xpp.setInput(new StringReader(packet));
			int eventType = xpp.getEventType();
			while (eventType != XmlPullParser.END_DOCUMENT) {
				type = xpp.getName();
				if (eventType == XmlPullParser.START_TAG) {
					if (type.equalsIgnoreCase("translator")) {
						if (xpp.next() == XmlPullParser.TEXT)
							p.translator = Integer.parseInt(xpp.getText());
					} else if (type.equalsIgnoreCase("accept")) {
						if (xpp.next() == XmlPullParser.TEXT) {
							p.accept = Integer.parseInt(xpp.getText()) != 0;
							accept_exist = true;
						}
					} else if (type.equalsIgnoreCase("phone")) {
						if (xpp.next() == XmlPullParser.TEXT)
							p.phone = xpp.getText();
					} else 
						;
				}
				eventType = xpp.next();
			}
			if (p.translator == 0 || !accept_exist || (p.accept && p.phone == null))
				return null;
		} catch (Exception ex) {
			return null;
		}
		return p;
	}
	class PacketPhonecallTimeout {
		int translator;
		int client;
	}
	public PacketPhonecallTimeout parsePhonecallTimeout(String packet) {
		PacketPhonecallTimeout p = new PacketPhonecallTimeout();
		String type;
		try {
			xpp.setInput(new StringReader(packet));
			int eventType = xpp.getEventType();
			while (eventType != XmlPullParser.END_DOCUMENT) {
				type = xpp.getName();
				if (eventType == XmlPullParser.START_TAG) {
					if (type.equalsIgnoreCase("translator")) {
						if (xpp.next() == XmlPullParser.TEXT)
							p.translator = Integer.parseInt(xpp.getText());
					} else if (type.equalsIgnoreCase("client")) {
						if (xpp.next() == XmlPullParser.TEXT)
							p.client = Integer.parseInt(xpp.getText());
					} else
						;
				}
				eventType = xpp.next();
			}
			if (p.translator == 0 && p.client == 0)
				return null;
		} catch (Exception ex) {
			return null;
		}
		return p;
	}
	public User parseTList_translator() throws Exception {
		int eventType = xpp.getEventType();
		int depth = xpp.getDepth();
		User t = new User();
		do {
			String type = xpp.getName();
			if (eventType == XmlPullParser.START_TAG) {
				if (type.equalsIgnoreCase("delete")) {
					if (xpp.next() == XmlPullParser.TEXT)
						t.delete = Integer.parseInt(xpp.getText()) != 0;
				} else if (type.equalsIgnoreCase("busy")) {
					if (xpp.next() == XmlPullParser.TEXT)
						t.busy = Integer.parseInt(xpp.getText()) != 0;
				} else if (type.equalsIgnoreCase("id")) {
					if (xpp.next() == XmlPullParser.TEXT)
						t.id = Integer.parseInt(xpp.getText());
				} else if (type.equalsIgnoreCase("name")) {
					if (xpp.next() == XmlPullParser.TEXT)
						t.name = xpp.getText();
				} else if (type.equalsIgnoreCase("rating")) {
					if (xpp.next() == XmlPullParser.TEXT)
						t.rating = Integer.parseInt(xpp.getText());
				} else if (type.equalsIgnoreCase("rating_num")) {
					if (xpp.next() == XmlPullParser.TEXT)
						t.rating_num = Integer.parseInt(xpp.getText());
				} else if (type.equalsIgnoreCase("translate")) {
					String lang = null;
					int price = -1;
					int tdepth = xpp.getDepth();
					do {
						if (xpp.getEventType() == XmlPullParser.START_TAG) {
							if (xpp.getName().compareToIgnoreCase("lang") == 0) {
								if (xpp.next() == XmlPullParser.TEXT)
									lang = xpp.getText();
							} else if (xpp.getName().compareToIgnoreCase("price") == 0) {
								if (xpp.next() == XmlPullParser.TEXT)
									price = Integer.parseInt(xpp.getText());
							}
							else
								;
						}
						xpp.next();
					} while (xpp.getDepth() > tdepth);
					if (lang == null|| !Lang.isLang(lang) || price < 0)
						return null;
					t.translate.put(lang, price);
				} else if (type.equalsIgnoreCase("client_lang")) {
					if (xpp.next() == XmlPullParser.TEXT)
						t.client_lang = xpp.getText();
				} else if (type.equalsIgnoreCase("price")) {
					if (xpp.next() == XmlPullParser.TEXT)
						t.price = Integer.parseInt(xpp.getText());
				} else if (type.equalsIgnoreCase("country")) {
					if (xpp.next() == XmlPullParser.TEXT)
						t.country = xpp.getText();
                } else if (type.equalsIgnoreCase("await")) {
                    if (xpp.next() == XmlPullParser.TEXT)
                        t.await = (Integer.parseInt(xpp.getText()) != 0);
                } else if (type.equalsIgnoreCase("confirmed")) {
                    if (xpp.next() == XmlPullParser.TEXT)
                        t.confirmed = (Integer.parseInt(xpp.getText()) != 0);
                } else if (type.equalsIgnoreCase("rejected")) {
                    if (xpp.next() == XmlPullParser.TEXT)
                        t.rejected = (Integer.parseInt(xpp.getText()) != 0);
                } else if (type.equalsIgnoreCase("error")) {
                    if (xpp.next() == XmlPullParser.TEXT)
                        t.error = (Integer.parseInt(xpp.getText()) != 0);
				} else if (type.equalsIgnoreCase("phone")) {
                    if (xpp.next() == XmlPullParser.TEXT)
                        t.phone = xpp.getText();
                } else
					;
			}
			eventType = xpp.next();		
		} while (xpp.getDepth() > depth);
		return t;
	}

    class PacketTList {
        ArrayList<User> tlist = new ArrayList<User>();
        int translators = 0;
    }
	public PacketTList parseTList(String packet) {
        PacketTList p = new PacketTList();
		String type;
		try {
			xpp.setInput(new StringReader(packet));
			int eventType = xpp.getEventType();
			while (eventType != XmlPullParser.END_DOCUMENT) {
				type = xpp.getName();
				if (eventType == XmlPullParser.START_TAG) {
                    if (type.equalsIgnoreCase("translator")) {
                        User t = parseTList_translator();
                        if (t == null)
                            return null;
                        if (!Country.isCountry(t.country))
                            t.country = Country.UNKNOWN;
                        p.tlist.add(t);
                    } else if (type.equalsIgnoreCase("translators")) {
                        if (xpp.next() == XmlPullParser.TEXT)
                            p.translators = Integer.parseInt(xpp.getText());
                    }
                }
				eventType = xpp.next();
			}
		} catch (Exception ex) {
			return null;
		}
		return p;
	}
    public User parseCList_client() throws Exception {
        int eventType = xpp.getEventType();
        int depth = xpp.getDepth();
        User c = new User();
        do {
            String type = xpp.getName();
            if (eventType == XmlPullParser.START_TAG) {
                if (type.equalsIgnoreCase("id")) {
                    if (xpp.next() == XmlPullParser.TEXT)
                        c.id = Integer.parseInt(xpp.getText());
                } else if (type.equalsIgnoreCase("delete")) {
                    if (xpp.next() == XmlPullParser.TEXT)
                        c.delete = Integer.parseInt(xpp.getText()) != 0;
                } else if (type.equalsIgnoreCase("name")) {
                    if (xpp.next() == XmlPullParser.TEXT)
                        c.name = xpp.getText();
                } else if (type.equalsIgnoreCase("country")) {
                    if (xpp.next() == XmlPullParser.TEXT)
                        c.country = xpp.getText();
                } else if (type.equalsIgnoreCase("client_lang")) {
                    if (xpp.next() == XmlPullParser.TEXT)
                        c.client_lang = xpp.getText();
                } else if (type.equalsIgnoreCase("translate_lang")) {
                    if (xpp.next() == XmlPullParser.TEXT)
                        c.translate_lang = xpp.getText();
                } else if (type.equalsIgnoreCase("balance")) {
                    if (xpp.next() == XmlPullParser.TEXT)
                        c.balance = Integer.parseInt(xpp.getText());
                } else if (type.equalsIgnoreCase("price")) {
                    if (xpp.next() == XmlPullParser.TEXT)
                        c.price = Integer.parseInt(xpp.getText());
                } else if (type.equalsIgnoreCase("confirmed")) {
                    if (xpp.next() == XmlPullParser.TEXT)
                        c.confirmed = Integer.parseInt(xpp.getText()) != 0;
                } else if (type.equalsIgnoreCase("rejected")) {
                    if (xpp.next() == XmlPullParser.TEXT)
                        c.rejected = (Integer.parseInt(xpp.getText()) != 0);
                } else if (type.equalsIgnoreCase("error")) {
                    if (xpp.next() == XmlPullParser.TEXT)
                        c.error = (Integer.parseInt(xpp.getText()) != 0);
                } else
                    ;
            }
            eventType = xpp.next();
        } while (xpp.getDepth() > depth);
        return c;
    }
    class PacketCList {
        ArrayList<User> clist = new ArrayList<User>();
        int clients = 0;
    }
    public PacketCList parseCList(String packet) {
        PacketCList p = new PacketCList();
        String type;
        try {
            xpp.setInput(new StringReader(packet));
            int eventType = xpp.getEventType();
            while (eventType != XmlPullParser.END_DOCUMENT) {
                type = xpp.getName();
                if (eventType == XmlPullParser.START_TAG) {
                    if (type.equalsIgnoreCase("client")) {
                        User c = parseCList_client();
                        if (c == null)
                            return null;
                        if (!Country.isCountry(c.country))
                            c.country = Country.UNKNOWN;
                        p.clist.add(c);
                    }
                }
                eventType = xpp.next();
            }
        } catch (Exception ex) {
            return null;
        }
        return p;
    }
    public User parseUserData(String packet, Options options) {
		if (options == null)
			return null;
		User user = new User();
		String type;
		try {
			xpp.setInput(new StringReader(packet));
			int eventType = xpp.getEventType();
			boolean is_translator_exist = false;
			while (eventType != XmlPullParser.END_DOCUMENT) {
				type = xpp.getName();
				if (eventType == XmlPullParser.START_TAG) {
					if (type.equalsIgnoreCase("is_translator")) {
						if (xpp.next() == XmlPullParser.TEXT) {
							user.isT = Integer.parseInt(xpp.getText()) > 0;
							is_translator_exist = true;
						}
					} else if (type.equalsIgnoreCase("email")) {
						if (xpp.next() == XmlPullParser.TEXT)
							user.email = xpp.getText();
					} else if (type.equalsIgnoreCase("name")) {
						if (xpp.next() == XmlPullParser.TEXT)
							user.name = xpp.getText();
					} else if (type.equalsIgnoreCase("phone_status")) {
						if (xpp.next() == XmlPullParser.TEXT)
							user.phone_status = Integer.parseInt(xpp.getText());
					} else if (type.equalsIgnoreCase("phone")) {
						if (xpp.next() == XmlPullParser.TEXT)
							user.phone = (xpp.getText());
                    } else if (type.equalsIgnoreCase("await_phone")) {
                        if (xpp.next() == XmlPullParser.TEXT)
                            user.await_phone = (xpp.getText());
					} else if (type.equalsIgnoreCase("balance")) {
						if (xpp.next() == XmlPullParser.TEXT)
							user.balance = Integer.parseInt(xpp.getText());
					} else if (type.equalsIgnoreCase("lang")) {
						if (xpp.next() == XmlPullParser.TEXT)
							user.lang = xpp.getText();
					} else if (type.equalsIgnoreCase("translate")) {
						int depth = xpp.getDepth();
						int price = -1;
						String lang = "";
						do {
							if (xpp.getEventType() == XmlPullParser.START_TAG) {
								if (xpp.getName().compareToIgnoreCase("lang") == 0) {
									if (xpp.next() == XmlPullParser.TEXT)
										lang = xpp.getText();
								} else if (xpp.getName().compareToIgnoreCase("price") == 0) {
									if (xpp.next() == XmlPullParser.TEXT)
										price = Integer.parseInt(xpp.getText());
								} else
									;
							}
							xpp.next();
						} while (xpp.getDepth() > depth);
						if (price  == -1 || lang.length() == 0)
							return null;
						user.translate.put(lang, price);
					} else if (type.equalsIgnoreCase("rating")) {
						if (xpp.next() == XmlPullParser.TEXT)
							user.rating = Integer.parseInt(xpp.getText());
					} else if (type.equalsIgnoreCase("rating_num")) {
						if (xpp.next() == XmlPullParser.TEXT)
							user.rating_num = Integer.parseInt(xpp.getText());
					} else if (type.equalsIgnoreCase("paypal_email")) {
						if (xpp.next() == XmlPullParser.TEXT)
							user.paypal_email = xpp.getText();
					} else if (type.equalsIgnoreCase("options")) {
						int depth = xpp.getDepth();
						do {
							if (xpp.getEventType() == XmlPullParser.START_TAG) {
								if (xpp.getName().compareToIgnoreCase("fee_market") == 0) {
									if (xpp.next() == XmlPullParser.TEXT)
										options.FeeMarket = Integer.parseInt(xpp.getText());
								} else if (xpp.getName().compareToIgnoreCase("fee_app") == 0) {
									if (xpp.next() == XmlPullParser.TEXT)
										options.FeeApp = Integer.parseInt(xpp.getText());
                                } else if (xpp.getName().compareToIgnoreCase("call_free_time_sec") == 0) {
                                    if (xpp.next() == XmlPullParser.TEXT)
                                        options.CallTimeFree = Integer.parseInt(xpp.getText());
                                } else if (xpp.getName().compareToIgnoreCase("call_min_balance_sec") == 0) {
                                    if (xpp.next() == XmlPullParser.TEXT)
                                        options.CallMinBalance = Integer.parseInt(xpp.getText());
                                } else if (xpp.getName().compareToIgnoreCase("call_min_time_rating_sec") == 0) {
                                    if (xpp.next() == XmlPullParser.TEXT)
                                        options.CallMinTimeRating = Integer.parseInt(xpp.getText());
								} else if (xpp.getName().compareToIgnoreCase("active_tsearch") == 0) {
                                    if (xpp.next() == XmlPullParser.TEXT)
                                        options.ActiveTSearch = Integer.parseInt(xpp.getText()) > 0;
                                }  else
									;
							}
							xpp.next();
						} while (xpp.getDepth() > depth);
					} else
						;
				}
				eventType = xpp.next();
			}
			if (user.name == null || user.name.compareToIgnoreCase("null") == 0)
				user.name = "";
			if (user.email == null || user.email.compareToIgnoreCase("null") == 0)
				user.email = "";
			if (!is_translator_exist || user.email.length() == 0)
				return null;
			if (!user.isT) {
				user.lang = Lang.DEFAULT;
				Iterator<Map.Entry<String, Integer>> it = user.translate.entrySet().iterator();
				if (it.hasNext())
					user.lang = it.next().getKey();
			}
		} catch (Exception ex) {
			return null;
		}
		return user;
	}
	class PacketMarkRequest {
		int translator;
		String name;
		Date time;
	}
	public PacketMarkRequest parseMarkRequest(String packet) {
		PacketMarkRequest p = new PacketMarkRequest();
		String type;
		try {
			xpp.setInput(new StringReader(packet));
			int eventType = xpp.getEventType();
			while (eventType != XmlPullParser.END_DOCUMENT) {
				type = xpp.getName();
				if (eventType == XmlPullParser.START_TAG) {
					if (type.equalsIgnoreCase("translator")) {
						if (xpp.next() == XmlPullParser.TEXT)
							p.translator = Integer.parseInt(xpp.getText());
					} else if (type.equalsIgnoreCase("name")) {
						if (xpp.next() == XmlPullParser.TEXT)
							p.name = xpp.getText();
					} else if (type.equalsIgnoreCase("time")) {
						if (xpp.next() == XmlPullParser.TEXT) {
							long ms = Long.parseLong(xpp.getText());
							p.time = new Date();
							p.time.setTime(ms);
						}
					} else
						;
				}
				eventType = xpp.next();
			}
			if (p.translator == 0 || p.name == null)
				return null;
		} catch (Exception ex) {
			return null;
		}
		return p;
	}
	public class PacketStatistic {
		public int clients;
		public int translators;
		public HashMap<String, Integer> language_stat;
		public int users_hour, users_day;
        public int calls_hour, calls_day;
		public PacketStatistic() {
			language_stat = new HashMap<String, Integer>();
		}
	}
	public PacketStatistic parseStatistic(String packet) {
		PacketStatistic p = new PacketStatistic();
		String type;
		try {
			xpp.setInput(new StringReader(packet));
			int eventType = xpp.getEventType();
			while (eventType != XmlPullParser.END_DOCUMENT) {
				type = xpp.getName();
				if (eventType == XmlPullParser.START_TAG) {
					if (type.equalsIgnoreCase("clients")) {
						if (xpp.next() == XmlPullParser.TEXT)
							p.clients = Integer.parseInt(xpp.getText());
					} else if (type.equalsIgnoreCase("translators")) {
						if (xpp.next() == XmlPullParser.TEXT)
							p.translators = Integer.parseInt(xpp.getText());
                    } else if (type.equalsIgnoreCase("users_hour")) {
                        if (xpp.next() == XmlPullParser.TEXT)
                            p.users_hour = Integer.parseInt(xpp.getText());
                    } else if (type.equalsIgnoreCase("users_day")) {
                        if (xpp.next() == XmlPullParser.TEXT)
                            p.users_day = Integer.parseInt(xpp.getText());
					} else if (type.equalsIgnoreCase("calls_hour")) {
						if (xpp.next() == XmlPullParser.TEXT)
							p.calls_hour = Integer.parseInt(xpp.getText());
                    } else if (type.equalsIgnoreCase("calls_day")) {
                        if (xpp.next() == XmlPullParser.TEXT)
                            p.calls_day = Integer.parseInt(xpp.getText());
					} else if (type.equalsIgnoreCase("language_stat")) {
						int depth = xpp.getDepth();
						int translators = -1;
						String lang = "";
						do {
							if (xpp.getEventType() == XmlPullParser.START_TAG) {
								if (xpp.getName().compareToIgnoreCase("lang") == 0) {
									if (xpp.next() == XmlPullParser.TEXT)
										lang = xpp.getText();
								} else if (xpp.getName().compareToIgnoreCase("translators") == 0) {
									if (xpp.next() == XmlPullParser.TEXT)
										translators = Integer.parseInt(xpp.getText());
								} else
									;
							}
							xpp.next();
						} while (xpp.getDepth() > depth);
						if (translators == -1 || lang.length() == 0)
							return null;
						p.language_stat.put(lang, translators);
					} else
						;
				}
				eventType = xpp.next();
			}
		} catch (Exception ex) {
			return null;
		}
		return p;
	}
    public class PacketTranslatorStatistic {
        public int all_rating_num;
        public int higher_rating_num;
        public int money_sum;
        public int call_num;
        public int call_time_sum;
    }
    public PacketTranslatorStatistic parseTranslatorStatistic(String packet) {
        PacketTranslatorStatistic p = new PacketTranslatorStatistic();
        String type;
        try {
            xpp.setInput(new StringReader(packet));
            int eventType = xpp.getEventType();
            while (eventType != XmlPullParser.END_DOCUMENT) {
                type = xpp.getName();
                if (eventType == XmlPullParser.START_TAG) {
                    if (type.equalsIgnoreCase("all_rating_num")) {
                        if (xpp.next() == XmlPullParser.TEXT)
                            p.all_rating_num = Integer.parseInt(xpp.getText());
                    } else if (type.equalsIgnoreCase("higher_rating_num")) {
                        if (xpp.next() == XmlPullParser.TEXT)
                            p.higher_rating_num = Integer.parseInt(xpp.getText());
                    } else if (type.equalsIgnoreCase("money_sum")) {
                        if (xpp.next() == XmlPullParser.TEXT)
                            p.money_sum = Integer.parseInt(xpp.getText());
                    } else if (type.equalsIgnoreCase("call_num")) {
                        if (xpp.next() == XmlPullParser.TEXT)
                            p.call_num = Integer.parseInt(xpp.getText());
                    } else if (type.equalsIgnoreCase("call_time_sum")) {
                        if (xpp.next() == XmlPullParser.TEXT)
                            p.call_time_sum = Integer.parseInt(xpp.getText());
                    } else
                        ;
                }
                eventType = xpp.next();
            }
        } catch (Exception ex) {
            return null;
        }
        return p;
    }
    public Call parseCallHistory_Call() throws Exception {
        int eventType = xpp.getEventType();
        int depth = xpp.getDepth();
        Call c = new Call();
        c.translator = new User();
        c.client = new User();
        do {
            String type = xpp.getName();
            if (eventType == XmlPullParser.START_TAG) {
                if (type.equalsIgnoreCase("translator")) {
                    if (xpp.next() == XmlPullParser.TEXT)
                        c.translator.id = Integer.parseInt(xpp.getText());
                } else if (type.equalsIgnoreCase("name")) {
                    if (xpp.next() == XmlPullParser.TEXT)
                        c.translator.name = xpp.getText();
                } else if (type.equalsIgnoreCase("lang")) {
                    if (xpp.next() == XmlPullParser.TEXT)
                        c.TranslateLang = xpp.getText();
                } else if (type.equalsIgnoreCase("price")) {
                    if (xpp.next() == XmlPullParser.TEXT)
                        c.price = Integer.parseInt(xpp.getText());
                } else if (type.equalsIgnoreCase("cost")) {
                    if (xpp.next() == XmlPullParser.TEXT)
                        c.cost = Integer.parseInt(xpp.getText());
                } else if (type.equalsIgnoreCase("start")) {
                    if (xpp.next() == XmlPullParser.TEXT) {
                        long s = Long.parseLong(xpp.getText());
                        c.start = new Date();
                        c.start.setTime(s*1000);
/*                        SimpleDateFormat format = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
                        try {
                            c.start = format.parse(xpp.getText());
                        } catch (Exception ex) {
                            c.start = null;
                        }
                        */
                    }
                } else if (type.equalsIgnoreCase("length")) {
                    if (xpp.next() == XmlPullParser.TEXT)
                        c.length = Integer.parseInt(xpp.getText());
                } else if (type.equalsIgnoreCase("client_country")) {
                    if (xpp.next() == XmlPullParser.TEXT)
                        c.client.country = xpp.getText();
                } else if (type.equalsIgnoreCase("translator_country")) {
                    if (xpp.next() == XmlPullParser.TEXT)
                        c.translator.country = xpp.getText();
                } else
                    ;
            }
            eventType = xpp.next();
        } while (xpp.getDepth() > depth);
        return c;
    }
    public ArrayList<Call> parseCallHistory(String packet) {
        ArrayList<Call> callList = new ArrayList<Call>();
        String type;
        try {
            xpp.setInput(new StringReader(packet));
            int eventType = xpp.getEventType();
            while (eventType != XmlPullParser.END_DOCUMENT) {
                type = xpp.getName();
                if (eventType == XmlPullParser.START_TAG) {
                    if (type.equalsIgnoreCase("call")) {
                        Call c = parseCallHistory_Call();
                        if (c == null)
                            return null;
                        if (!Country.isCountry(c.client.country))
                            c.client.country = Country.UNKNOWN;
                        callList.add(c);
                    }
                }
                eventType = xpp.next();
            }
        } catch (Exception ex) {
            return null;
        }
        return callList;
    }
    public User parseMarkHistory_Translator() throws Exception {
        int eventType = xpp.getEventType();
        int depth = xpp.getDepth();
        User t = new User();
        do {
            String type = xpp.getName();
            if (eventType == XmlPullParser.START_TAG) {
                if (type.equalsIgnoreCase("id")) {
                    if (xpp.next() == XmlPullParser.TEXT)
                        t.id = Integer.parseInt(xpp.getText());
                } else if (type.equalsIgnoreCase("name")) {
                    if (xpp.next() == XmlPullParser.TEXT)
                        t.name = xpp.getText();
                } else if (type.equalsIgnoreCase("rating")) {
                    if (xpp.next() == XmlPullParser.TEXT)
                        t.rating = Integer.parseInt(xpp.getText());
                } else if (type.equalsIgnoreCase("rating_num")) {
                    if (xpp.next() == XmlPullParser.TEXT)
                        t.rating_num = Integer.parseInt(xpp.getText());
                } else if (type.equalsIgnoreCase("deleted")) {
                    if (xpp.next() == XmlPullParser.TEXT)
                        t.delete = Integer.parseInt(xpp.getText()) != 0;
                } else
                    ;
            }
            eventType = xpp.next();
        } while (xpp.getDepth() > depth);
        return t;
    }
    public ArrayList<User> parseMarkHistory(String packet) {
        ArrayList<User> markList = new ArrayList<User>();
        String type;
        try {
            xpp.setInput(new StringReader(packet));
            int eventType = xpp.getEventType();
            while (eventType != XmlPullParser.END_DOCUMENT) {
                type = xpp.getName();
                if (eventType == XmlPullParser.START_TAG) {
                    if (type.equalsIgnoreCase("translator")) {
                        User t = parseMarkHistory_Translator();
                        if (t == null)
                            return null;
                        markList.add(t);
                    }
                }
                eventType = xpp.next();
            }
        } catch (Exception ex) {
            return null;
        }
        return markList;
    }
	public int parseLangs(Lang group, ArrayList<Lang> langs) throws Exception {
		String type;
		int eventType = xpp.getEventType();
		int depth = xpp.getDepth();
		do {
			type = xpp.getName();
			if (eventType == XmlPullParser.START_TAG) {
				if (type.equalsIgnoreCase("lang")) {
					Lang l = new Lang();
					l.isGroup = false;
					int ldepth = xpp.getDepth();
					do {
						if (xpp.getEventType() == XmlPullParser.START_TAG) {
							if (xpp.getName().compareToIgnoreCase("code") == 0) {
								if (xpp.next() == XmlPullParser.TEXT)
									l.code = xpp.getText();
							} else if (xpp.getName().compareToIgnoreCase("name") == 0) {
								if (xpp.next() == XmlPullParser.TEXT)
									l.name = xpp.getText();
							} else if (xpp.getName().compareToIgnoreCase("iso3") == 0) {
								if (xpp.next() == XmlPullParser.TEXT)
									l.iso3 = xpp.getText();
							} else if (xpp.getName().compareToIgnoreCase("country") == 0) {
								if (xpp.next() == XmlPullParser.TEXT)
									l.country = xpp.getText();
							} else if (xpp.getName().compareToIgnoreCase("native") == 0) {
								if (xpp.next() == XmlPullParser.TEXT)
									l.nativeName = xpp.getText();
							} else if (xpp.getName().compareToIgnoreCase("avg_price") == 0) {
								if (xpp.next() == XmlPullParser.TEXT)
									l.avg_price = Integer.parseInt(xpp.getText());
							} else
								;
						}
						xpp.next();
					} while (xpp.getDepth() > ldepth);
					if (l.code == null|| l.name == null)
						return -1;
					if (l.nativeName.length() == 0)
						l.nativeName = l.name;
					l.group = group;
					langs.add(l);
				} else if (type.equalsIgnoreCase("group")) {
					Lang g = new Lang();
					g.isGroup = true;
					if (xpp.getAttributeCount() < 1)
						return -1;
					for (int i=0; i<xpp.getAttributeCount(); i++)
						if (xpp.getAttributeName(i).compareTo("name") == 0) {
							g.name = xpp.getAttributeValue(i);
						} else if (xpp.getAttributeName(i).compareTo("code") == 0) {
							g.code = xpp.getAttributeValue(i);
						} 
					if (g.name.length() == 0 || g.code.length() == 0)
						return -1;
					g.group = group;
					langs.add(g);
					xpp.next();
					parseLangs(g, langs);
				}
			}
			eventType = xpp.next();		
		} while (xpp.getDepth() >= depth);
		return 0;
	}
	public int parseLangsPacket(String LangsXML, ArrayList<Lang> langs) {
		String type;
		try {
			xpp.setInput(new StringReader(LangsXML));
			int eventType = xpp.getEventType();
			while (eventType != XmlPullParser.END_DOCUMENT) {
				type = xpp.getName();
				if (eventType == XmlPullParser.START_TAG) {
					if (type.equalsIgnoreCase("languages")) {
						return parseLangs(null, langs);
					}
				}
				eventType = xpp.next();
			}
		} catch (Exception ex) {
			return -1;
		}
		return 0;
	}
	public int parseLangsFile(InputStream file, ArrayList<Lang> langs) {
		String type;
		try {
			xpp.setInput(new InputStreamReader(file));
			int eventType = xpp.getEventType();
			while (eventType != XmlPullParser.END_DOCUMENT) {
				type = xpp.getName();
				if (eventType == XmlPullParser.START_TAG) {
					if (type.equalsIgnoreCase("languages")) {
						return parseLangs(null, langs);
					}
				}
				eventType = xpp.next();
			}
		} catch (Exception ex) {
			return -1;
		}
		return 0;
	}
	public int parseLDict(Lang group, ArrayList<Lang> langs) throws Exception {
		String type;
		int eventType = xpp.getEventType();
		int depth = xpp.getDepth();
		do {
			type = xpp.getName();
			if (eventType == XmlPullParser.START_TAG) {
				if (type.equalsIgnoreCase("lang") || type.equalsIgnoreCase("group")) {
					String code = null;
					HashMap<String, String> dict = new HashMap<String, String>(); 
					int ldepth = xpp.getDepth();
					xpp.next();
					do {
						if (xpp.getEventType() == XmlPullParser.START_TAG) {
							String n = xpp.getName();
//							if (xpp.getName().compareToIgnoreCase("code") == 0) {
							if (n.compareToIgnoreCase("code") == 0) {
								
							if (xpp.next() == XmlPullParser.TEXT)
									code = xpp.getText();
							} else {
								String name_code = xpp.getName();
								if (xpp.next() == XmlPullParser.TEXT)
									dict.put(name_code, xpp.getText());
							}
						}
						xpp.next();
					} while (xpp.getDepth() > ldepth);
					if (code == null)
						continue;
					for (int i=0; i<langs.size(); i++) {
						Lang l = langs.get(i);
						if (l.code.compareToIgnoreCase(code) == 0) {
							l.dict = dict;
							break;
						}
					}
				}
			}
			eventType = xpp.next();		
		} while (xpp.getDepth() >= depth);
		return 0;
	}
	public int parseLDictFile(InputStream file, ArrayList<Lang> langs) {
		String type;
		try {
			xpp.setInput(new InputStreamReader(file));
			int eventType = xpp.getEventType();
			while (eventType != XmlPullParser.END_DOCUMENT) {
				type = xpp.getName();
				if (eventType == XmlPullParser.START_TAG) {
					if (type.equalsIgnoreCase("lang_dict")) {
						return parseLDict(null, langs);
					}
				}
				eventType = xpp.next();
			}
		} catch (Exception ex) {
			return -1;
		}
		return 0;
	}
	public int parseCountries(ArrayList<Country> countries) throws Exception {
		String type;
		int eventType = xpp.getEventType();
		int depth = xpp.getDepth();
		while (eventType != XmlPullParser.END_DOCUMENT) {
			type = xpp.getName();
			if (eventType == XmlPullParser.START_TAG) {
				if (type.equalsIgnoreCase("country")) {
					Country c = new Country();
					int cdepth = xpp.getDepth();
					do {
						if (xpp.getEventType() == XmlPullParser.START_TAG) {
							if (xpp.getName().compareToIgnoreCase("code") == 0) {
								if (xpp.next() == XmlPullParser.TEXT)
									c.code = xpp.getText();
							} else if (xpp.getName().compareToIgnoreCase("name") == 0) {
								if (xpp.next() == XmlPullParser.TEXT)
									c.name = xpp.getText();
							} else if (xpp.getName().compareToIgnoreCase("iso3") == 0) {
								if (xpp.next() == XmlPullParser.TEXT)
									c.iso3 = xpp.getText();
							} else if (xpp.getName().compareToIgnoreCase("lang") == 0) {
								if (xpp.next() == XmlPullParser.TEXT)
									c.lang = xpp.getText();
							} else if (xpp.getName().compareToIgnoreCase("langs") == 0) {
								if (xpp.next() == XmlPullParser.TEXT)
									c.langs = xpp.getText();
							} else
								;
						}
						xpp.next();
					} while (xpp.getDepth() > cdepth);
					if (c.code == null|| c.name == null || c.lang == null)
						return -1;
//					if (!Lang.isLang(c.lang))
//						c.lang = Lang.DEFAULT;
					countries.add(c);
				}
			}
			eventType = xpp.next();		
		} while (xpp.getDepth() >= depth);
		return 0;
	}
	public int parseCountriesFile(InputStream file, ArrayList<Country> countries) {
		String type;
		try {
			xpp.setInput(new InputStreamReader(file));
			int eventType = xpp.getEventType();
			while (eventType != XmlPullParser.END_DOCUMENT) {
				type = xpp.getName();
				if (eventType == XmlPullParser.START_TAG) {
					if (type.equalsIgnoreCase("countries")) {
						return parseCountries(countries);
					}
				}
				eventType = xpp.next();
			}
		} catch (Exception ex) {
			return -1;
		}
		return 0;
	}	
	public Parser() {
		xpp = Xml.newPullParser();
		try {
			xpp.setFeature(XmlPullParser.FEATURE_PROCESS_NAMESPACES, true);
		} catch (Exception ex) { 
		}
	}
	private XmlPullParser xpp;
}
