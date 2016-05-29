package com.translatenet;

import java.lang.reflect.Field;

import android.graphics.Color;
import android.net.Network;
import android.text.SpannableStringBuilder;
import android.text.TextUtils;
import android.os.RemoteException;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Stack;
import java.util.Timer;
import java.util.TimerTask;
import org.json.JSONObject;
import org.json.JSONException;
import com.android.vending.billing.IInAppBillingService;
import android.accounts.Account;
import android.accounts.AccountManager;
import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.annotation.TargetApi;
import android.app.AlertDialog;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.media.MediaPlayer;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentTransaction;
import android.support.v4.app.NotificationCompat;
import android.support.v7.app.ActionBar;
import android.support.v7.app.ActionBarActivity;
import android.telephony.TelephonyManager;
import android.text.format.DateFormat;
import android.text.style.ForegroundColorSpan;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.ArrayAdapter;
import android.widget.EditText;
import android.widget.RemoteViews;
import android.widget.TextView;
import android.widget.Toast;

public class MainActivity extends ActionBarActivity {
    ActionBarActivity this_activity = this;
    ActionBar actionBar;
    boolean ActivityCreated = false;

    public static final String PreferencesName = "TranslateNetPreferences";
    public static final String Preference_LastEmail = "LastLogin";
    public static final String Preference_LastPassword = "LastPassword";
    public static final String Preference_Country = "Country";
    public static final String Preference_ListLang = "ListLang";
    public static final String Preference_Money = "Money";
    public static final String File_Cert = "translate_cert.pem";
    public static final String File_CACert = "ca.pem";
    public static final String File_Lang = "langs.xml";
    public static final String File_LDict = "ldict.xml";
    public static final String File_Countries = "countries.xml";
    public static final int TIMEOUT_LOGIN = 20000;

    public static boolean DEBUG_EMU = false;
    Parser parser = new Parser();
    boolean Connected = false;
    public static boolean WAS_LOGGEDIN = false;
    boolean BusyWhileLoad = true;
    boolean AnotherLogin = false;
    public static boolean ReceivedError_PhoneChanged = false;

    User user;
    static boolean isT;

    public User getUser() {
        return user;
    }

    String UILang = Lang.DEFAULT;
    String CountryLang = Lang.DEFAULT;
    PhoneContacts phoneContacts = new PhoneContacts();
    static Options options = new Options();
    public static Options getOptions() {
        return options;
    }
    public static int SMS_SentNum = 0, SMS_BlockDays = 0;

    public static String curCountry = null;
    TNGeoLocation geoLocation;
    Parser.PacketStatistic Stat;

    StatusFragment statusFragment;
    SettingsFragment settingsFragment;
    TListFragment tlistFragment;
    boolean unknownCall = false;
    boolean approvedCall = false;           // received valid phoncall_status while being called from this number
    PhonecallFragment phonecallFragment;
    CListFragment clistFragment;
    SettingsUserFragment settingsUserFragment;
    SettingsLangFragment settingsLangFragment;
    SettingsTranslateFragment settingsTranslateFragment;
    SettingsPhoneFragment settingsPhoneFragment;
    HistoryFragment historyFragment;
    StatisticFragment statisticFragment;
    ArrayList<Fragment> AddedFragment = new ArrayList<Fragment>();
    Stack<Fragment> FragmentStack = new Stack<Fragment>();

    void UserUpdateCTListFragments() {
        if (!user.isT) {
            if (tlistFragment != null && FragmentStack.contains(tlistFragment)) {
                tlistFragment.setClient(user);
                tlistFragment.UpdateTranslators();
            }
        } else {
            if (clistFragment != null && FragmentStack.contains(clistFragment)) {
                clistFragment.setTranslator(user);
                clistFragment.updateTranslatorView();
                clistFragment.UpdateClients();
            }
        }
    }

    public void setUser(User _user) {
        if (_user == null)
            return;
        try {
            user = (User) _user.clone();
        } catch (Exception ex) {
        }

        isT = _user.isT;

//		user = _user;
        UserUpdateCTListFragments();
    }

    boolean Reported_NetworkError = false;

    protected void onError(String err) {
        if (!Reported_NetworkError) {
            Toast toast = Toast.makeText(getApplicationContext(), err, Toast.LENGTH_SHORT);
            toast.show();
//			UserMessage(getString(R.string.error), err);
        }
        Reported_NetworkError = true;
    }

    protected int onPacketReceived(String packet) {
        Reported_NetworkError = false;
        String type = parser.parseType(packet);
        if (type == null || type.compareToIgnoreCase("") == 0) {
            UserMessage(getString(R.string.error), getString(R.string.network_error_format));
            showProgress(false);
            return -1;
        }
        if (type.compareToIgnoreCase("user_data") == 0) {
            User _user = parser.parseUserData(packet, options);
            if (_user == null) {
                Toast toast = Toast.makeText(getApplicationContext(), getErrorMessage(getApplicationContext(), Parser.ERROR_OTHER), Toast.LENGTH_LONG);
                toast.show();
                Disconnect(false);
                return -1;
            }

            if (settingsUserFragment != null)
                settingsUserFragment.setUser(_user);
            startPhonecallListener();

            setUser(_user);

            if (assertUserData() == 0) {
                if (!_user.CheckLangs()) {
                    if (_user.isT)
                        UserMessage(getString(R.string.error), getString(R.string.error_translate));
                    else {
                        _user.lang = Lang.DEFAULT;
                        if (_user.name != null && _user.name.length() > 0)
                            UserMessage(getString(R.string.error), getString(R.string.error_lang) + getLangNameByCode(_user.lang));
                    }
                    sendPacket_UserData(user);
                }
                if (ReceivedError_PhoneChanged)
                    onErrorPhoneChanged();
            }
            showProgress(false);

            if (!user.isT)
                ConnectToBillingService();

            if (BusyWhileLoad) {
//				if (user.isT)
                sendPacket_SetBusy(false);
                BusyWhileLoad = false;
            }
            return 0;
        }
        if (user == null)
            return -1;
        if (type.compareToIgnoreCase("error") == 0) {
            Parser.PacketError p = parser.parseError(packet);
            if (p == null)
                return -1;
            onPacketError(p);
        } else if (type.compareToIgnoreCase("statistic") == 0) {
            Stat = parser.parseStatistic(packet);
            if (Stat == null)
                return -1;
            if (user == null)
                return -1;
            UpdateStatistic(Stat);
            if (statisticFragment != null)
                if (FragmentStack.contains(statisticFragment)) {
                    statisticFragment.setStat(Stat);
                }

//			if (statusFragment != null)
//				statusFragment.updateStatistic(stat, user);
        } else if (type.compareToIgnoreCase("translator_statistic") == 0) {
            Parser.PacketTranslatorStatistic TStat = parser.parseTranslatorStatistic(packet);
            if (TStat == null)
                return -1;
            if (user == null)
                return -1;
            if (statisticFragment != null)
                if (FragmentStack.contains(statisticFragment)) {
                    statisticFragment.setTStat(TStat);
                }
        } else if (type.compareToIgnoreCase("translator_list") == 0) {
            Parser.PacketTList p = parser.parseTList(packet);
            if (p == null)
                return -1;
            if (tlistFragment != null)
                tlistFragment.onPacket_TranslatorList(p.tlist);
            Stat.translators = p.translators;
            UpdateStatistic(Stat);
        } else if (type.compareToIgnoreCase("client_list") == 0) {
            Parser.PacketCList p = parser.parseCList(packet);
            if (p == null)
                return -1;
            if (clistFragment != null)
                clistFragment.onPacket_ClientList(p.clist);
            Stat.clients = p.clients;
            UpdateStatistic(Stat);
        } else if (type.compareToIgnoreCase("await_phone_confirm") == 0) {
            settingsPhoneFragment.onPacket_AwaitPhoneConfirm();
        } else if (type.compareToIgnoreCase("phonecall_status") == 0) {
            Parser.PacketPhonecallStatus p = parser.parsePhonecallStatus(packet);
            if (p == null)
                return -1;
            onPacketPhonecallStatus(p);
        } else if (type.compareToIgnoreCase("phonecall_confirm") == 0) {
            if (user.isT)
                return -1;
            Parser.PacketPhonecallConfirm p = parser.parsePhonecallConfirm(packet);
            if (p == null)
                return -1;
            if (tlistFragment != null)
                if (tlistFragment.onPacketPhonecallConfirm(p) == 0) {
/*                                              done during NetworkClient preprocessing
                    MediaPlayer mPlayer;
                    if (p.accept)
					    mPlayer = MediaPlayer.create(MainActivity.this, R.raw.call_confirm);
                    else
                        mPlayer = MediaPlayer.create(MainActivity.this, R.raw.call_reject);
					mPlayer.start();
					*/
                }
        } else if (type.compareToIgnoreCase("phonecall_request") == 0) {
            if (!user.isT)
                return -1;
            Parser.PacketPhonecallRequest p = parser.parsePhonecallRequest(packet);
            if (p == null)
                return -1;
            if (!Lang.isLang(p.client_lang) || !Lang.isLang(p.translate_lang) || !Country.isCountry(p.country)) {
                sendPacket_PhonecallConfirm(p.client, false);
                return 0;
            }
            if (clistFragment != null)
                if (clistFragment.onPacketPhonecallRequest(p)) {
                    /*                              done during NetworkClient preprocessing
					MediaPlayer mPlayer = MediaPlayer.create(MainActivity.this, R.raw.call_request);
					mPlayer.start();
					*/
                }
            Toast toast = Toast.makeText(this, getString(R.string.phonecall_request), Toast.LENGTH_LONG);
            toast.show();
        } else if (type.compareToIgnoreCase("phonecall_timeout") == 0) {
            Parser.PacketPhonecallTimeout p = parser.parsePhonecallTimeout(packet);
            if (p == null)
                return -1;
            if (user.isT && p.client != 0 && clistFragment != null) {
                clistFragment.onPacketPhonecallTimeout(p);
            } else if (!user.isT && p.translator != 0 && tlistFragment != null) {
                tlistFragment.onPacketPhonecallTimeout(p);
                if (FragmentStack.contains(phonecallFragment))
                    ClosePhonecallFragment();
            } else
                return -1;
        } else if (type.compareToIgnoreCase("mark_request") == 0) {
            Parser.PacketMarkRequest p = parser.parseMarkRequest(packet);
            if (p == null)
                return -1;
            OpenMarkDialog(p);
        } else if (type.compareToIgnoreCase("call_history") == 0) {
            ArrayList<Call> p = parser.parseCallHistory(packet);
            if (p == null)
                return -1;
            historyFragment.onPacketCallHistory(p);
        } else if (type.compareToIgnoreCase("mark_history") == 0) {
            ArrayList<User> p = parser.parseMarkHistory(packet);
            if (p == null)
                return -1;
            historyFragment.onPacketMarkHistory(p);
        }
        return 0;
    }

    protected int onPacketError(Parser.PacketError p) {
        if (p == null)
            return -1;
        if (p.code == Parser.ERROR_RATING_ERROR)
            return 0;
        if (p.code == Parser.ERROR_VERSION) {
            String msg = "";
            if (p.message != null && p.message.length() > 0)
                msg = p.message;
            UserMessage(MainActivity.getErrorMessage(getApplicationContext(), p.code), msg);
            return 0;
        }
        if (p.code == Parser.ERROR_PEER_DISCON) {
//            else if (p.code == Parser.ERROR_PEER_DISCON && (FragmentStack.peek() == clistFragment || FragmentStack.peek() == tlistFragment)) {
            Toast toast = Toast.makeText(getApplicationContext(),
                    getErrorMessage(getApplicationContext(), p.code) + "(" + p.message + ")",
                    Toast.LENGTH_LONG);
            toast.show();
            if (clistFragment != null && FragmentStack.contains(clistFragment))
                clistFragment.onPacketPhonecallError(p.code, p.id);
            return 0;
        }
        if (p.command != null) {
            if (p.command.compareToIgnoreCase("set_country") == 0) {
                if (p.code == Parser.ERROR_NOERROR)
                    WAS_LOGGEDIN = true;
                else
                    Disconnect(false);
            } else if (p.command.compareToIgnoreCase("user_data") == 0) {
                if (settingsUserFragment != null && FragmentStack.contains(settingsUserFragment) && !settingsUserFragment.isHidden()) {
                    if (settingsUserFragment.onPacketErrorUserData(p.code))
                        return 0;
                } else if (settingsLangFragment != null && FragmentStack.contains(settingsLangFragment) && !settingsLangFragment.isHidden()) {
                    if (settingsLangFragment.onPacketErrorUserData(p.code))
                        return 0;
                } else if (settingsTranslateFragment != null && FragmentStack.contains(settingsTranslateFragment) && !settingsTranslateFragment.isHidden()) {
                    if (settingsTranslateFragment.onPacketErrorUserData(p.code))
                        return 0;
                } else if (tlistFragment != null && FragmentStack.contains(tlistFragment) && !tlistFragment.isHidden()) {
                    if (tlistFragment.onPacketErrorUserData(p.code))
                        return 0;
                }
                assertUserData();
                return 0;
            } else if (p.command.compareToIgnoreCase("billing") == 0) {
               if (settingsUserFragment != null) {
                    if (settingsUserFragment.BillingPacketError(p.code))
                        return 0;
                }
            } else if (p.command.compareToIgnoreCase("paypal_transfer") == 0) {
                if (settingsUserFragment != null) {
                    if (settingsUserFragment.PayPalTransferPacketError(p.code))
                        return 0;
                }
            } else if (p.command.compareToIgnoreCase("phonecall_request") == 0) {
                if (/*FragmentStack.peek() == tlistFragment && */p.phonecall_request_translator != 0) {
                    if (tlistFragment != null)
                        if (tlistFragment.onPacket_PhonecallError(p.code, p.phonecall_request_translator))
                            return 0;
                    if (phonecallFragment != null)
                        if (phonecallFragment.onError_Phonecall(p.code, p.phonecall_request_translator))
                            return 0;
                } else if (/*curFragment == clistFragment && */p.phonecall_request_client != 0) {
                    if (phonecallFragment != null)
                        if (phonecallFragment.onError_Phonecall(p.code, p.phonecall_request_client))
                            return 0;
                }
            } else if (p.command.compareTo("phonecall_confirm") == 0) {     // not implemented in server
                if (/*curFragment == clistFragment && */p.phonecall_request_client != 0)
                    if (clistFragment != null)
                        if (clistFragment.onPacketPhonecallError(p.code, p.phonecall_request_client))
                            return 0;
            } else if (p.command.compareToIgnoreCase("stop_translator_list") == 0) {
                if (tlistFragment != null)
                    if (tlistFragment.onErrorStopTranslatorList(p.code))
                        return 0;
            } else if (p.command.compareToIgnoreCase("register_phone") == 0) {
                if (settingsPhoneFragment.getAutoDetermined()) {
                    if (p.code == Parser.ERROR_NOERROR)
                        UserMessage(getString(R.string.settings), getString(R.string.new_phone_msg) + user.phone);
                    else
                        UserMessage(getString(R.string.settings_error), getString(R.string.phone_update_error));
                    return 0;
                } else
                    if (settingsPhoneFragment.onPacketError_RegisterPhone(p))
                        return 0;
            } else if (p.command.compareToIgnoreCase("confirm_register_phone") == 0) {
                if (settingsPhoneFragment != null) {
                    if (settingsPhoneFragment.onPacketError_ConfirmRegisterPhone(p))
                        return 0;
                }
            } else if (p.command.compareToIgnoreCase("resend_sms") == 0) {
                if (settingsPhoneFragment != null) {
                    if (settingsPhoneFragment.onPacketError_ResendSMS(p))
                        return 0;
                }
            } else if (p.command.compareToIgnoreCase("mark_rating") == 0) {
                if (p.code == 0 && historyFragment != null) {
                    if (FragmentStack.contains(historyFragment))
                        historyFragment.FragmentOpened();
                }
            }
        }
        if (p.code != Parser.ERROR_NOERROR) {
            if (p.code == Parser.ERROR_NO_USERDATA)
                return 0;
            else if (p.code == Parser.ERROR_ANOTHER_LOGIN) {
                AnotherLogin = true;
//                UserMessage(getString(R.string.error), getErrorMessage(getApplicationContext(), Parser.ERROR_ANOTHER_LOGIN));
                Toast toast = Toast.makeText(getApplicationContext(), getErrorMessage(getApplicationContext(), Parser.ERROR_ANOTHER_LOGIN), Toast.LENGTH_LONG);
                toast.show();
                WAS_LOGGEDIN = false;
                return 0;
            } else if (p.code == Parser.ERROR_PHONE_CHANGED) {
                onErrorPhoneChanged();
                return 0;
            } else {
                UserMessage(getString(R.string.error), getErrorMessage(getApplicationContext(), p.code));
                if (p.code == Parser.ERROR_UNKNOWN_COUNTRY)
                    Disconnect(false);
                return 0;
            }
        }
        return 0;
    }

    protected int onPacketPhonecallStatus(Parser.PacketPhonecallStatus p) {
        Call call = new Call();
        call.client = new User();
        call.translator = new User();
        call.initFromPacket(p, user);

        boolean wrongPhoneNumber = !call.active;
        if (call.active) {
            if (!user.isT && call.translator.phone != null)
                wrongPhoneNumber = call.translator.phone.equalsIgnoreCase(phonecallNumber);
            else if (call.client.phone != null)
                wrongPhoneNumber = call.client.phone.equalsIgnoreCase(phonecallNumber);
        }
        if ((wrongPhoneNumber && phonecallStart != null && /**/!phonecallFragment.isCallActive())
                && !approvedCall) {
            unknownCall = true;
            if (user.isT) {
                phonecallFragment.setAlertStarted();
                if (!Notified_CLIENT_UNKNOWN_CALL) {
                    NotifyInactiveCall(NOTIFY_UNKNOWN_CALL);
                    Notified_CLIENT_UNKNOWN_CALL = true;
                }
            }
        }

        if (p.active) {
            if (FragmentStack.contains(tlistFragment))
                if (call.active)
                    tlistFragment.onCallStarted(p.peer); // might be client
            if (FragmentStack.contains(phonecallFragment)) {
                Call prev_call = phonecallFragment.getCall();
                if (prev_call != null && prev_call.client != null &&
                        prev_call.client.balance < call.client.balance) {
                    MediaPlayer mPlayer = MediaPlayer.create(MainActivity.this, R.raw.update_balance);
                    mPlayer.start();
                }
            }
            if (!unknownCall)
                approvedCall = true;
        } else {
            call.active = false;
            call.length = p.time;
//				if (p.time > 0) {
//					phonecallFragment.CallFinished(p.time);
            if (user.isT && FragmentStack.contains(clistFragment))
                if (p.peer > 0 && p.time > 0)
                    clistFragment.removeClientItem(p.peer);
            // else
        }

        if (call.active || phonecallFragment.isCallActive() || phonecallStart != null) {
            if (FragmentStack.peek() != phonecallFragment) {
                if (user.isT || !unknownCall)
                    OpenPhonecallFragment(call, false);
            } else
                phonecallFragment.setCall(call);
        }
        return 0;
    }


    void onErrorPhoneChanged() {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                UserMessage(getString(R.string.note), getErrorMessage(getApplicationContext(), Parser.ERROR_PHONE_CHANGED));
            }
        });
        ReceivedError_PhoneChanged = false;
    }

    void UpdateStatistic(Parser.PacketStatistic stat) {
        if (user.isT)
            actionBar.setTitle(getString(R.string.clients_hour) + ((Integer) stat.users_hour).toString());
        else
            actionBar.setTitle(getString(R.string.translators) + ((Integer) stat.translators).toString());
        if (statisticFragment != null)
            if (FragmentStack.contains(statisticFragment)) {
                statisticFragment.setStat(stat);
            }
    }

    public int sendPacket_GetStatistic() {
        return NC_sendPacket("<get_statistic></get_statistic>");
    }

    public int sendPacket_GetTranslatorStatistic() {
        return NC_sendPacket("<get_translator_statistic></get_translator_statistic>");
    }

    public int sendPacket_SetBusy(boolean busy) {
        String packet = "<set_busy>";
        packet += "<busy>";
        packet += busy ? 1 : 0;
        packet += "</busy>";
        packet += "</set_busy>";
        return NC_sendPacket(packet);
    }

    public int sendPacket_GetUserData() {
        return NC_sendPacket("<get_user_data></get_user_data>");
    }

    public int sendPacket_UserData(User u) {
        String packet = "<user_data>";
        if (u.name != null) {
            packet += "<name>";
            packet += u.name;
            packet += "</name>";
        }
        if (user.isT) {
            Iterator it = u.translate.entrySet().iterator();
            while (it.hasNext()) {
                Map.Entry<String, Integer> t = (Map.Entry<String, Integer>) it.next();
                packet += "<translate>";
                packet += "<lang>";
                packet += t.getKey();
                packet += "</lang>";
                packet += "<price>";
                packet += t.getValue();
                packet += "</price>";
                packet += "</translate>";
            }
            if (u.paypal_email != null) {
                packet += "<paypal_email>";
                packet += u.paypal_email;
                packet += "</paypal_email>";
            }
        } else {
            if (u.lang != null && Lang.isLang(u.lang)) {
                packet += "<translate>";
                packet += "<lang>";
                packet += u.lang;
                packet += "</lang>";
                packet += "<price>";
                packet += "0";
                packet += "</price>";
                packet += "</translate>";
            }
        }
        packet += "</user_data>";
        return NC_sendPacket(packet);
    }

    public int sendPacket_Billing(int money, String data, String signature) {
        String packet = "<billing>";
        packet += "<money>";
        packet += money;
        packet += "</money>";
        if (data.length() > 0) {
            packet += "<data>";
            packet += data;
            packet += "</data>";
        }
        if (signature.length() > 0) {
            packet += "<signature>";
            packet += signature;
            packet += "</signature>";
        }
        packet += "</billing>";
        return NC_sendPacket(packet);
    }

    public int sendPacket_PayPalTransfer(int money) {
        String packet = "<paypal_transfer>";
        packet += "<money>";
        packet += money;
        packet += "</money>";
        packet += "</paypal_transfer>";
        return NC_sendPacket(packet);
    }

    public int sendPacket_PhonecallStatus(boolean active, String number, long time) {
        String packet = "<phonecall_status>";
        packet += "<active>";
        packet += active ? 1 : 0;
        packet += "</active>";
        if (number != null && number.length() > 0) {
            packet += "<peer_phone>";
            packet += number;
            packet += "</peer_phone>";
        }
        packet += "<time>";
        packet += time;
        packet += "</time>";
        packet += "</phonecall_status>";
        return NC_sendPacket(packet);
    }

    public int sendPacket_PhonecallRequest(int translator, String translate_lang) {
        String packet = "<phonecall_request>";
        packet += "<translator>";
        packet += translator;
        packet += "</translator>";
        packet += "<translate_lang>";
        packet += translate_lang;
        packet += "</translate_lang>";
        packet += "</phonecall_request>";
        return NC_sendPacket(packet);
    }

    public int sendPacket_PhonecallConfirm(int client, boolean accept) {
        String packet = "<phonecall_confirm>";
        packet += "<client>";
        packet += client;
        packet += "</client>";
        packet += "<accept>";
        packet += accept ? 1 : 0;
        packet += "</accept>";
        packet += "</phonecall_confirm>";
        return NC_sendPacket(packet);
    }

    public int sendPacket_RequestTranslatorList(String list_lang) {
        String packet = "<request_translator_list>";
        packet += "<list_lang>";
        packet += list_lang;
        packet += "</list_lang>";
        packet += "</request_translator_list>";
        return NC_sendPacket(packet);
    }

    public int sendPacket_StopTranslatorList() {
        String packet = "<stop_translator_list></stop_translator_list>";
        return NC_sendPacket(packet);
    }

    public int sendPacket_RequestClientList() {
        String packet = "<request_client_list>";
        packet += "</request_client_list>";
        return NC_sendPacket(packet);
    }

    public int sendPacket_MarkRating(int translator, int mark) {
        String packet = "<mark_rating>";
        packet += "<translator>";
        packet += translator;
        packet += "</translator>";
        packet += "<mark>";
        packet += mark;
        packet += "</mark>";
        packet += "</mark_rating>";
        return NC_sendPacket(packet);
    }

    public int sendPacket_RegisterPhone(String phone, int user_input, String device_id) {
        if (phone == null)// || device_id == null)
            return -1;
        String packet = "<register_phone>";
        packet += "<phone>";
        packet += phone;
        packet += "</phone>";
        packet += "<user_input>";
        packet += user_input;
        packet += "</user_input>";
//		packet += "<device_id>";	packet += device_id;	packet += "</device_id>";
        packet += "</register_phone>";
        return NC_sendPacket(packet);
    }

    public int sendPacket_ConfirmRegisterPhone(String code) {
        if (code == null)
            return -1;
        String packet = "<confirm_register_phone>";
        packet += "<sms_code>";
        packet += code;
        packet += "</sms_code>";
        packet += "</confirm_register_phone>";
        return NC_sendPacket(packet);
    }

    public int sendPacket_ResendSMS() {
        String packet = "<resend_sms>";
        packet += "</resend_sms>";
        return NC_sendPacket(packet);
    }

    public int sendPacket_GetCallHistory() {
        return NC_sendPacket("<get_call_history></get_call_history>");
    }

    public int sendPacket_GetMarkHistory() {
        return NC_sendPacket("<get_mark_history></get_mark_history>");
    }

    public int sendPacket_RegisterUser(String email) {
        return NC_sendPacket("<register_user><email>" + email + "</email></register_user>");
    }

    public int sendPacket_ResetPassword() {
        return NC_sendPacket("<reset_password></reset_password>");
    }


    public int NC_sendPacket_SetCountry() {
        if (!mNetworkBound)
            return -1;
        Message msg = Message.obtain(null, NetworkClient.ACTION_SEND_SET_COUNTRY, 0, 0);
        Bundle b = new Bundle();
        msg.setData(b);
        if (mService == null)
            return -1;
        try {
            mService.send(msg);
        } catch (Exception ex) {
            UserMessage(getString(R.string.error), "System error.");
            Log.e("Error", "[" + ex.toString() + "]:" + ex.getMessage());
        }
        return 0;

    }

    public int NC_sendPacket(String packet) {
        if (!mNetworkBound)
            return -1;
        Message msg = Message.obtain(null, NetworkClient.ACTION_SEND, 0, 0);
        Bundle b = new Bundle();
        b.putString(NetworkClient.PARAM_MSG, packet);
        msg.setData(b);
        if (mService == null)
            return -1;
        try {
            mService.send(msg);
        } catch (Exception ex) {
            UserMessage(getString(R.string.error), "System error.");
            Log.e("Error", "[" + ex.toString() + "]:" + ex.getMessage());
        }
        return 0;
    }


    public int assertUserData() {
//			OpenSettingsLang();
        if (user.name == null || user.name.length() == 0) {
//			if (!user.isT && (user.lang == null || user.lang.length() == 0 || user.lang.compareToIgnoreCase(Lang.UNKNOWN) == 0)) {
            if (!user.isT) {
                user.lang = getCurrentLanguage();
//				OpenSettingsLang();
//				return false;
            }
            OpenSettingsUser();
            return 1;
        } else if (user.isT && (user.translate == null || user.translate.size() < 2)) {
            OpenSettingsTranslate();
            return 1;
        } else if (user.phone_status != User.PHONE_STATUS_CONFIRMED ||
                user.phone == null || user.phone.length() == 0) {
            OpenSettingsPhone();
            return 1;
        }
        onUserDataLoaded();
        return 0;
    }

    Date phonecallStart;
    String phonecallNumber;

    protected void onCallEnded(String number, long length) {
        if (Connected)
            sendPacket_PhonecallStatus(false, number, length);
        if (user != null)
            if (!user.isT && tlistFragment != null && FragmentStack.contains(tlistFragment)) {
;//                tlistFragment.CallFinished(number);
            }
        phonecallStart = null;
        phonecallNumber = null;
    }

    protected void onIncomingCallRinging(Context ctx, String number, Date start) {
    }

    protected void onIncomingCallOffHook(Context ctx, String number, Date start) {
        phonecallNumber = number;
        approvedCall = false;
        if (!phoneContacts.isPhoneInContact(phonecallNumber))
            phonecallStart = new Date(System.currentTimeMillis());
        else
            phonecallStart = null;
        Notified_CLIENT_UNKNOWN_CALL = false;
        sendPacket_PhonecallStatus(true, number, 0);
/*		if (user.isT) {
			Call call = new Call();
			call.client = null;
			call.translator = null;
			if (FragmentStack.peek() != phonecallFragment)
				OpenPhonecallFragment(call, false);
			else
				phonecallFragment.setCall(call);
		}
*/
    }

    protected void onIncomingCallEnded(Context ctx, String number, Date start, Date end) {
        approvedCall = false;
        if (phonecallStart != null) {
            Date now = new Date(System.currentTimeMillis());
            long length = (now.getTime() - phonecallStart.getTime()) / 1000;
            sendPacket_PhonecallStatus(true, number, length);
            onCallEnded(number, length);
            if (unknownCall) {
                if (phonecallFragment != null)
                    if (FragmentStack.contains(phonecallFragment))
                        phonecallFragment.CallFinished((int)length);
                unknownCall = false;
            }
        } else
            onCallEnded(number, 0);
    }

    protected void onOutgoingCallEnded(Context ctx, String number, Date start, Date end) {
        sendPacket_PhonecallStatus(true, number, 0);
        onCallEnded(number, 0);
    }

    protected void onMissedCall(Context ctx, String number, Date start) {
    }

    private static final int NOTIFY_UNKNOWN_CALL = 1;
    private static final int NOTIFY_CLIENT_LOW_BALANCE = 2;
    private static final int NOTIFY_LOW_BALANCE = 3;
    boolean Notified_CLIENT_UNKNOWN_CALL = false;
    boolean Notified_CLIENT_LOW_BALANCE = false;
    boolean Notified_LOW_BALANCE = false;
    private static final String CANCELALERT_EXTRA = "com.translatenet.CANCEL_ALERT";

    public void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        if (intent.getBooleanExtra(CANCELALERT_EXTRA, true)) {
            if (phonecallFragment != null && FragmentStack.contains(phonecallFragment))
                phonecallFragment.doCancelAlert();
            CancelNotifications();
        }
    }

    public static final String ACTION_NOTIF_CANCEL_ALERT = "com.translatenet.notification.CANCEL_ALERT";
    public BroadcastReceiver CallNotificationReceiver = new BroadcastReceiver() {
        public void onReceive(Context context, Intent intent) {
            if (phonecallFragment != null && FragmentStack.contains(phonecallFragment))
                phonecallFragment.doCancelAlert();
            CancelNotifications();
        }

        ;
    };

    protected void CancelNotifications() {
        NotificationManager nManager = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
        nManager.cancelAll();
    }

    protected void NotifyInactiveCall(int notify_type) {
        if (phonecallStart == null)
            return;
        NotificationManager nManager = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
        Intent intent = new Intent(this, MainActivity.class);
        intent.putExtra(CANCELALERT_EXTRA, true);
        intent.addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
        PendingIntent pIntent = PendingIntent.getActivity(this_activity, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT);
        NotificationCompat.Builder nb = new NotificationCompat.Builder(this)
                .setSmallIcon(R.drawable.ic_launcher)
                .setContentIntent(pIntent);
        switch (notify_type) {
            case NOTIFY_UNKNOWN_CALL:
                RemoteViews rViews = new RemoteViews(getPackageName(), R.layout.notification_call);
                Intent btnIntent = new Intent();
                btnIntent.setAction(ACTION_NOTIF_CANCEL_ALERT);
                btnIntent.putExtra(CANCELALERT_EXTRA, true);
                PendingIntent btnPIntent = PendingIntent.getBroadcast(this, 0, btnIntent, PendingIntent.FLAG_UPDATE_CURRENT);
                rViews.setOnClickPendingIntent(R.id.NC_CancelAlertButton, btnPIntent);
                nb.setContent(rViews);
                Date d = new Date();
//			String date = DateFormat.format("yy-mm-dd hh:mm", d.getTime()).toString();
                String date = FormatDate(d);
//			nb.setContentText();
                rViews.setTextViewText(R.id.NC_DescrTextView, getString(R.string.phonecall_unknown) + " " + date);
                nb.setContentTitle(getString(R.string.cancel_alert));
                break;
            case NOTIFY_CLIENT_LOW_BALANCE:
                nb.setContentText(getString(R.string.client_low_balance));
                nb.setContentTitle(getString(R.string.cancel_alert));
                break;
            case NOTIFY_LOW_BALANCE:
                nb.setContentText(getString(R.string.low_balance));
                nb.setContentTitle(getString(R.string.deposit_money));
                break;
        }
        nManager.notify(notify_type, nb.build());
    }

    Thread alertThread;
    public static final int TIMER_ALERT = 10000;
    public static final int MIN_BALANCE_ALERT = 200;

    protected void startAlertThread() {
        alertThread = new Thread(new Runnable() {
            public void run() {
                while (true) {
                    try {
                        if (Thread.currentThread().isInterrupted())
                            return;
                        if (phonecallFragment != null) {
                            if (user.isT && phonecallStart != null &&  /**/phonecallFragment.isAlertStarted()) {
//									System.currentTimeMillis() - phonecall_status_MAX_DELAY > phonecallStart.getTime()) {
                                if (!phonecallFragment.isCallActive()) {
                                    phonecallFragment.setAlertStarted();
                                    MediaPlayer mPlayer = MediaPlayer.create(MainActivity.this, R.raw.low_balance);
                                    mPlayer.start();
                                } else {
                                    Call call = phonecallFragment.getCall();
                                    if (call != null && call.client != null && call.client.balance <= MIN_BALANCE_ALERT) {
                                        phonecallFragment.setAlertStarted();
                                        if (!Notified_CLIENT_LOW_BALANCE) {
                                            NotifyInactiveCall(NOTIFY_CLIENT_LOW_BALANCE);
                                            Notified_CLIENT_LOW_BALANCE = true;
                                        }
                                        MediaPlayer mPlayer = MediaPlayer.create(MainActivity.this, R.raw.low_balance);
                                        mPlayer.start();
                                    } else {
                                        Notified_CLIENT_LOW_BALANCE = false;
                                        CancelNotifications();
                                    }
                                }
                            } else if (!user.isT && phonecallFragment.isCallActive()) {
                                if (user.balance <= MIN_BALANCE_ALERT) {
                                    if (!Notified_LOW_BALANCE) {
                                        NotifyInactiveCall(NOTIFY_LOW_BALANCE);
                                        Notified_LOW_BALANCE = true;
                                    }
                                    MediaPlayer mPlayer = MediaPlayer.create(MainActivity.this, R.raw.low_balance);
                                    mPlayer.start();
                                } else {
                                    Notified_CLIENT_LOW_BALANCE = false;
                                    CancelNotifications();
                                }
                            }
                        }
                        synchronized (this) {
                            wait(TIMER_ALERT);
                        }
                    } catch (InterruptedException ex) {
                        return;
                    } catch (Exception ex) {
                    }
                }
            }
        });
        alertThread.start();
    }

    protected void stopAlertThread() {
        if (alertThread != null)
            alertThread.interrupt();
        alertThread = null;
    }

    public void CancelAlert() {
        phonecallStart = null;
    }


    boolean mNetworkBound;
    Messenger mService = null;
    final Messenger mNClientMessenger = new Messenger(new IncomingHandler());

    class IncomingHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case NetworkClient.ACTION_REPLY:
                    if (msg.getData().containsKey(NetworkClient.PARAM_MSG))
                        onPacketReceived(msg.getData().getString(NetworkClient.PARAM_MSG));
                    else if (msg.getData().containsKey(NetworkClient.PARAM_ERR)) {
                        onError(msg.getData().getString(NetworkClient.PARAM_ERR));
                        //Disconnect();
                        Close();
                        Connected = false;
                    }
                    break;
                case NetworkClient.ACTION_CON:
                    Connected = msg.getData().getBoolean(NetworkClient.PARAM_CON);
                    if (!Connected) {
                        showProgress(false);
//					if (statusFragment != null)
//						statusFragment.updateLoaded(false);
                        if (AnotherLogin) {
                            AnotherLogin = false;
                        } else {
                            Toast toast = Toast.makeText(getApplicationContext(), getString(R.string.network_error_connection), Toast.LENGTH_SHORT);
                            toast.show();
                        }
                        Close();
                    }
                    break;
                default:
                    super.handleMessage(msg);
                    break;
            }
        }
    }

    private ServiceConnection mNClientConnection = new ServiceConnection() {
        public void onServiceConnected(ComponentName className, IBinder service) {
            // This is called when the connection with the service has been
            // established, giving us the object we can use to
            // interact with the service.  We are communicating with the
            // service using a Messenger, so here we get a client-side
            // representation of that from the raw IBinder object.
            mService = new Messenger(service);
            mNetworkBound = true;
            Message msg = Message.obtain(null, NetworkClient.ACTION_REG, 0, 0);
            msg.replyTo = mNClientMessenger;
            try {
                mService.send(msg);
            } catch (Exception ex) {
                UserMessage("Error", "System error.");
                Log.e("Error", ex.toString() + ":" + ex.getMessage());
            }

            //if (!Country.isCountry(curCountry))
                InitSendCurrentCountry();

            sendPacket_GetUserData();
        }

        public void onServiceDisconnected(ComponentName className) {
            // This is called when the connection with the service has been
            // unexpectedly disconnected -- that is, its process crashed.
            mService = null;
            mNetworkBound = false;
        }
    };

    protected void UnregNetworkClient() {
        Message msg = Message.obtain(null, NetworkClient.ACTION_UNREG, 0, 0);
        msg.replyTo = mNClientMessenger;
        try {
            mService.send(msg);
        } catch (Exception ex) {
            UserMessage("Error", "System error.");
            Log.e("Error", ex.toString() + ":" + ex.getMessage());
        }
    }

    protected void Connect() {
        Message msg = Message.obtain(null, NetworkClient.ACTION_CON, 0, 0);
        try {
            mService.send(msg);
        } catch (Exception ex) {
//			UserMessage(getString(R.string.error), "System error.");
            Log.e(getString(R.string.error), ex.toString() + ":" + ex.getMessage());
        }
    }

    protected void Disconnect(boolean TryRelogin) {
        WAS_LOGGEDIN = TryRelogin && (user != null && user.isT);
        Message msg = Message.obtain(null, NetworkClient.ACTION_DISCON, 0, 0);
        try {
            mService.send(msg);
        } catch (Exception ex) {
//			UserMessage(getString(R.string.error), "System error.");
            Log.e(getString(R.string.error), ex.toString() + ":" + ex.getMessage());
        }
    }

    //	public static final String Billing_1credit = "android.test.purchased";
    public static final String Billing_1credit = "1credit";
    //    public static final String Billing_2credit = "2credit";
    public static final String Billing_2credit = "android.test.purchased";
    public static final String Billing_5credit = "5credit";
    public static final String Billing_10credit = "10credit";
    public static final String Billing_25credit = "25credit";
    IInAppBillingService mBillingService = null;
    ServiceConnection mBillingServiceCon = new ServiceConnection() {
        @Override
        public void onServiceDisconnected(ComponentName name) {
            mBillingService = null;
        }

        ;

        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            mBillingService = IInAppBillingService.Stub.asInterface(service);
            ConsumeAllPurchases();
        }

        ;
    };
    public static final int BILLING_RESPONSE_RESULT_OK = 0;
    public static final int BILLING_RESPONSE_RESULT_USER_CANCELED = 1;
    public static final int BILLING_RESPONSE_RESULT_SERVICE_UNAVAILABLE = 2;
    public static final int BILLING_RESPONSE_RESULT_BILLING_UNAVAILABLE = 3;
    public static final int BILLING_RESPONSE_RESULT_ITEM_UNAVAILABLE = 4;
    public static final int BILLING_RESPONSE_RESULT_DEVELOPER_ERROR = 5;
    public static final int BILLING_RESPONSE_RESULT_ERROR = 6;
    public static final int BILLING_RESPONSE_RESULT_ITEM_ALREADY_OWNED = 7;
    public static final int BILLING_RESPONSE_RESULT_ITEM_NOT_OWNED = 8;
    public static final int BILLING_EXCEPTION = 16;
    public static final String RESPONSE_CODE = "RESPONSE_CODE";
    public static final String RESPONSE_GET_SKU_DETAILS_LIST = "DETAILS_LIST";
    public static final String RESPONSE_BUY_INTENT = "BUY_INTENT";
    public static final String RESPONSE_INAPP_PURCHASE_DATA = "INAPP_PURCHASE_DATA";
    public static final String RESPONSE_INAPP_SIGNATURE = "INAPP_DATA_SIGNATURE";
    public static final String RESPONSE_INAPP_ITEM_LIST = "INAPP_PURCHASE_ITEM_LIST";
    public static final String RESPONSE_INAPP_PURCHASE_DATA_LIST = "INAPP_PURCHASE_DATA_LIST";
    public static final String RESPONSE_INAPP_SIGNATURE_LIST = "INAPP_DATA_SIGNATURE_LIST";
    public static final String INAPP_CONTINUATION_TOKEN = "INAPP_CONTINUATION_TOKEN";

    void ConsumeAllPurchases() {
        String continuationToken = "";
        boolean hasMorePurchases = false;
        do {
            try {
                Bundle purchases = mBillingService.getPurchases(3, getPackageName(), "inapp", continuationToken);
                if (purchases == null)
                    return;
                int response = purchases.getInt("RESPONSE_CODE");
                if (response == 0) {
                    continuationToken = purchases.getString("INAPP_CONTINUATION_TOKEN");
                    if (!TextUtils.isEmpty(continuationToken)) hasMorePurchases = true;
                    final ArrayList<String> purchaseDataList = purchases.getStringArrayList("INAPP_PURCHASE_DATA_LIST");
                    ArrayList<String> signatureList = purchases.getStringArrayList("INAPP_DATA_SIGNATURE_LIST");
//                    for(String purchaseJSON : purchaseDataList) {
                    for (int i = 0; i < purchaseDataList.size(); i++) {
                        String purchaseJSON = purchaseDataList.get(i);
                        JSONObject object = new JSONObject(purchaseJSON);
                        String productId = object.getString("productId");
                        String orderId = object.getString("orderId");
                        String purchaseToken = object.getString("purchaseToken");
                        Log.i(getClass().getSimpleName(), "consuming purchase of " + productId + ", orderId " + orderId);
                        String signature = signatureList.get(i);
                        settingsUserFragment.BillingDone(productId, true, purchaseJSON, signature);
//    	                mBillingService.consumePurchase(3, getPackageName(), purchaseToken);
                        BillingConsume(purchaseToken);

/*                        runOnUiThread(new Runnable () {
                            public void run() {
                                UserMessage(getString(R.string.billing), getString(R.string.found_purchase));
                            }
                        }
                        */
                    }
                } else {
                    Log.e(getClass().getSimpleName(), "could not get purchases: " + response);
                }
            } catch (RemoteException e) {
                Log.e(getClass().getSimpleName(), "RemoteException during getPurchases:", e);
            } catch (JSONException e) {
                Log.e(getClass().getSimpleName(), "JSONException during getSkuDetails:", e);
            }
        } while (hasMorePurchases);
    }

    int getResponseCodeFromBundle(Bundle b) {
        Object o = b.get(RESPONSE_CODE);
        if (o == null) {
//            logDebug("Bundle with null response code, assuming OK (known issue)");
            return BILLING_RESPONSE_RESULT_OK;
        } else if (o instanceof Integer) return ((Integer) o).intValue();
        else if (o instanceof Long) return (int) ((Long) o).longValue();
        else {
//            logError("Unexpected type for bundle response code.");
            //           logError(o.getClass().getName());
            throw new RuntimeException("Unexpected type for bundle response code: " + o.getClass().getName());
        }
    }

    String getPurchaseTokenFromBundle(Bundle b) {
        String pdata = (String) b.get(RESPONSE_INAPP_PURCHASE_DATA);
        return getPurchaseToken(pdata);
    }

    private static final int RequestCode_Billing = 1001;

    public int BillingRequestBuy(String billingSKU) {
        try {
            Bundle buyIntentBundle = mBillingService.getBuyIntent(3, getPackageName(), billingSKU, "inapp", null);
            int response = getResponseCodeFromBundle(buyIntentBundle);
            switch (response) {
                case BILLING_RESPONSE_RESULT_OK:
                    break;
                case BILLING_RESPONSE_RESULT_ITEM_ALREADY_OWNED:
                    ConsumeAllPurchases();
/*				String purchaseData = (String)pendingIntent.get(RESPONSE_INAPP_PURCHASE_DATA);
				String token = getPurchaseToken(purchaseData);
				BillingConsume(token);
*/
                    return BILLING_RESPONSE_RESULT_OK;
                default:
                    return response;
            }
            PendingIntent pendingIntent = buyIntentBundle.getParcelable("BUY_INTENT");
            startIntentSenderForResult(pendingIntent.getIntentSender(), RequestCode_Billing, new Intent(),
                    Integer.valueOf(0), Integer.valueOf(0), Integer.valueOf(0));
        } catch (Exception ex) {
            return BILLING_EXCEPTION;
        }
        return BILLING_RESPONSE_RESULT_OK;
    }

    public void BillingRequestAddMoney(String billingSKU) {
        int response = BillingRequestBuy(billingSKU);
        if (response != BILLING_RESPONSE_RESULT_OK)
            UserMessage(getString(R.string.error_billing), getString(R.string.code) + String.valueOf(response));
    }

    protected String getPurchaseToken(String purchaseData) {
        try {
            JSONObject jo = new JSONObject(purchaseData);
            String sku = jo.getString("productId");
            String token = jo.optString("token", jo.optString("purchaseToken"));
            return token;
        } catch (JSONException e) {
            UserMessage(getString(R.string.error_billing), getString(R.string.code) + String.valueOf(BILLING_EXCEPTION));
        }
        return null;
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == RequestCode_Billing) {
            int responseCode = data.getIntExtra(RESPONSE_CODE, 0);
            String PurchaseData = data.getStringExtra(RESPONSE_INAPP_PURCHASE_DATA);
            String PurchaseSignature = data.getStringExtra(RESPONSE_INAPP_SIGNATURE);
            String ProductId = null;
            try {
                JSONObject object = new JSONObject(PurchaseData);
                ProductId = object.getString("productId");
            } catch (Exception ex) {
            }
            if (resultCode != RESULT_OK)
                settingsUserFragment.BillingDone(ProductId, false, PurchaseData, PurchaseSignature);
            else {
                BillingConsume(getPurchaseToken(PurchaseData));
                settingsUserFragment.BillingDone(ProductId, true, PurchaseData, PurchaseSignature);
            }
        }
        super.onActivityResult(requestCode, resultCode, data);
    }

    protected void BillingConsume(final String purchaseToken) {
//		UserMessage("Billing", billingSKU);
        showProgress(true);
        Runnable runnable = new Runnable() {
            @Override
            public void run() {
                class DoneRunnable implements Runnable {
                    int response;

                    DoneRunnable(int _response) {
                        response = _response;
                    }

                    @Override
                    public void run() {
                        showProgress(false);
                        if (response != BILLING_RESPONSE_RESULT_OK) {
                            UserMessage(getString(R.string.error_billing), getString(R.string.code) + String.valueOf(response));
//							settingsUserFragment.BillingDone(false, PurchaseData, PurchaseSignature);
                        } else
                            ;//							settingsUserFragment.BillingDone(true, PurchaseData, PurchaseSignature);
                    }
                }
                try {
                    int response = mBillingService.consumePurchase(3, getPackageName(), purchaseToken);
                    runOnUiThread(new DoneRunnable(response));
                } catch (Exception ex) {
                    runOnUiThread(new DoneRunnable(BILLING_EXCEPTION));
                }
            }

            ;
        };
        new Thread(runnable).start();
    }

    public int getSettingsBilling() {
        SharedPreferences settings = getSharedPreferences(user.email, 0);
        String MoneyStr = settings.getString(Preference_Money, "");
        if (MoneyStr == null || MoneyStr.length() == 0)
            return 0;
        return Integer.parseInt(MoneyStr);
    }

    public void storeSettingsBilling(int money) {
        SharedPreferences settings = getSharedPreferences(user.email, 0);
        SharedPreferences.Editor editor = settings.edit();
        editor.putString(Preference_Money, String.valueOf(money));
        editor.commit();
    }

    public interface LangCallback {
        void LangSelected(String lang);
    }

    boolean SelectLang_NativeName;

    public void SelectLang(Lang group, final LangCallback callback, boolean NativeName) {
        SelectLang_NativeName = NativeName;
        AlertDialog.Builder builderSingle = new AlertDialog.Builder(

                this_activity);
        builderSingle.setIcon(R.drawable.ic_launcher);
        builderSingle.setTitle("Select language");
        final ArrayAdapter<String> arrayAdapter = new ArrayAdapter<String>(
                this_activity,
                android.R.layout.select_dialog_singlechoice);
        for (int i = 0; i < Lang.Langs.size(); i++) {
            Lang l = Lang.Langs.get(i);
            if (l.group == group) {
                if (!NativeName || l.isGroup)
                    arrayAdapter.add(getLangNameByCode(l.code));
                else
                    arrayAdapter.add(Lang.CodeToNative(l.code));
            }
        }
        builderSingle.setNegativeButton("cancel",
                new DialogInterface.OnClickListener() {

                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        dialog.dismiss();
                    }
                });

        builderSingle.setAdapter(arrayAdapter,
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        String langName = arrayAdapter.getItem(which);
                        if (Lang.isGroup(langName)) {
                            dialog.dismiss();
                            SelectLang(Lang.getLangByName(langName), callback, SelectLang_NativeName);
                            return;
                        }
                        String code = getLangCodeByName(langName);
                        if (code.length() > 0) {
                            dialog.dismiss();
                            callback.LangSelected(code);
                            return;
                        }
                        dialog.dismiss();
                    }
                });
        builderSingle.show();
    }

    public String FormatDate(Date date) {
        return DateFormat.format("yy-MM-dd hh:mm", date.getTime()).toString();
    }

    public String FormatTime(int sec) {
        int hours = sec / 3600;
        int minutes = (sec - hours * 3600) / 60;
        int seconds = sec - hours * 3600 - minutes * 60;
        return String.format("%02d:%02d:%02d", hours, minutes, seconds);
    }

    public String FormatPrice(int money) {
        return String.format("%.2f", (float) money / 100) + " " + getString(R.string.credit);
    }

    public static String getDefaultEmail(Context context) {
        AccountManager manager = AccountManager.get(context);
        Account[] accounts = manager.getAccountsByType("com.google");
        List<String> possibleEmails = new LinkedList<String>();
        for (Account account : accounts) {
            // TODO: Check possibleEmail against an email regex or treat
            // account.name as an email address only for certain account.type values.
            if (account.name.matches(".+@.+\\..+"))
                possibleEmails.add(account.name);
        }

        if (!possibleEmails.isEmpty() && possibleEmails.get(0) != null)
            return possibleEmails.get(0);
        return null;
    }


    protected PhonecallReceiver phonecallReceiver = null;
    TimerTask phonecallStatusTask = null;
    Timer phonecallStatusTimer = null;
    public static final int CALL_STATUS_PERIOD = 5000;        // 5 seconds
    boolean phonecallReceiverRegistered = false;
    ;

    void stopPhonecallListener() {
        if (phonecallReceiverRegistered)
            unregisterReceiver(phonecallReceiver);
        phonecallReceiverRegistered = false;
    }

    void sendPhonecallStatus() {
        if (phonecallStart != null) {
            Date now = new Date(System.currentTimeMillis());
            sendPacket_PhonecallStatus(true, phonecallNumber, (now.getTime() - phonecallStart.getTime()) / 1000);
        } else
            sendPacket_PhonecallStatus(false, "", 0);
    }

    void startPhonecallListener() {
        if (phonecallReceiver != null)
            return;
        sendPhonecallStatus();

        phonecallReceiver = new PhonecallReceiver();
        phonecallReceiver.setActivity(this);
        IntentFilter phonestateFilter = new IntentFilter("android.intent.action.PHONE_STATE");
        registerReceiver(phonecallReceiver, phonestateFilter);
        IntentFilter outgoingCallsFilter = new IntentFilter("android.intent.action.NEW_OUTGOING_CALL");
        registerReceiver(phonecallReceiver, outgoingCallsFilter);
        phonecallReceiverRegistered = true;
        phonecallStatusTimer = new Timer();
        phonecallStatusTask = new TimerTask() {
            public void run() {
                if (phonecallStart != null) {
                    sendPhonecallStatus();
                }
            }
        };
        phonecallStatusTimer.schedule(phonecallStatusTask, 0, CALL_STATUS_PERIOD);
    }

    @Override
    public void onBackPressed() {
        // TODO Auto-generated method stub
//		super.onBackPressed();
        if (!FragmentStack.isEmpty()) {
            if (FragmentStack.peek() == tlistFragment || FragmentStack.peek() == clistFragment) {
                super.onBackPressed();
                Disconnect(false);
                return;
            }
            if (FragmentStack.peek() == phonecallFragment) {
                if (phonecallFragment.isCallActive())
                    return;
                phonecallFragment.FragmentClosed();
            }
            if (FragmentStack.peek() == settingsTranslateFragment)
                settingsTranslateFragment.FragmentClosed();
            Fragment f = FragmentStack.pop();
            if (!FragmentStack.isEmpty()) {
                if (f == settingsUserFragment || f == settingsPhoneFragment ||
                        f == settingsTranslateFragment || f == settingsLangFragment)
                    sendPacket_SetBusy(false);
                SwapFragment(f, FragmentStack.peek());
            } else {
                Disconnect(false);
            }
        } else {
            Disconnect(false);
        }
    }

    boolean ActivityStopped = false;
    boolean postOpenTListFragment = false;
    boolean postOpenCListFragment = false;
    boolean postOpenPhonecallFragment = false;
    boolean postClosePhonecallFragment = false;
    boolean postCloseSettingsUserFragment;
    boolean postCloseSettingsLangFragment;
    boolean postCloseSettingsTranslateFragment;
    boolean postCloseSettingsPhoneFragment;
    boolean postCloseHistoryFragment;
    boolean postCloseStatisticFragment;
    boolean postOpenMarkDialog;
    Parser.PacketMarkRequest postedMarkRequest;

    public void onUserDataLoaded() {
        if (user == null)
            return;
        supportInvalidateOptionsMenu();
        if (FragmentStack.contains(tlistFragment) || FragmentStack.contains(clistFragment))
            return;
        if (user.isT) {
            OpenCListFragment();
            clistFragment.setTranslator(user);
            clistFragment.EnableMenu();
        } else {
            OpenTListFragment();
            tlistFragment.EnableMenu();
        }
        UserUpdateCTListFragments();
        stopAlertThread();
        startAlertThread();
//		stopStatisticThread();
//		startStatisticThread();
        showProgress(false);
    }

    void doOpenTListFragment() {
        OpenFragment(tlistFragment);
//        tlistFragment.ActivityOnStart();
    }

    public void OpenTListFragment() {
        if (tlistFragment == null)
            return;
        if (!ActivityStopped)
            doOpenTListFragment();
        else {
            postOpenTListFragment = true;
        }
    }

    void doOpenCListFragment() {
        OpenFragment(clistFragment);
    }

    public void OpenCListFragment() {
        if (clistFragment == null)
            return;
        if (!ActivityStopped)
            doOpenCListFragment();
        else {
            postOpenCListFragment = true;
        }
    }

    void doOpenPhonecallFragment() {
        if (FragmentStack.contains(phonecallFragment))
            CloseFragment(phonecallFragment);
        OpenFragment(phonecallFragment);
        phonecallFragment.FragmentOpened();
        supportInvalidateOptionsMenu();
    }

    public void OpenPhonecallFragment(Call call, boolean makingCall) {
        if (phonecallFragment == null || call == null)
            return;
        phonecallFragment.setMakingCall(makingCall);
        phonecallFragment.setCall(call);
        if (!ActivityStopped)
            doOpenPhonecallFragment();
        else {
            postOpenPhonecallFragment = true;
            postClosePhonecallFragment = false;
        }
    }

    void doClosePhonecallFragment() {
        if (phonecallFragment == null)
            return;
        phonecallFragment.FragmentClosed();
        CloseFragment(phonecallFragment);
        supportInvalidateOptionsMenu();
    }

    public void ClosePhonecallFragment() {
        phonecallFragment.setCall(null);
        if (!ActivityStopped) {
            doClosePhonecallFragment();
        } else {
            postOpenPhonecallFragment = false;
            postClosePhonecallFragment = true;
        }
    }

    public void OpenSettingsUser() {
        if (FragmentStack.contains(settingsUserFragment) /*&& !settingsUserFragment.isHidden()*/)
            return;
        CloseSettingsFragments();
        settingsUserFragment.setUser(user);
        OpenFragment(settingsUserFragment);
    }

    protected void doCloseSettingsUser() {
        if (ActivityStopped)
            return;
        CloseFragment(settingsUserFragment);
        assertUserData();
    }

    public void CloseSettingsUser() {
        if (!ActivityStopped)
            doCloseSettingsUser();
        else
            postCloseSettingsUserFragment = true;
    }

    public void OpenSettingsLang() {
        if (FragmentStack.contains(settingsLangFragment)/* && !settingsLangFragment.isHidden()*/)
            return;
        CloseSettingsFragments();
        if (!user.isT)
            settingsLangFragment.setClient(user);
        sendPacket_SetBusy(true);
        OpenFragment(settingsLangFragment);
    }

    protected void doCloseSettingsLang() {
        if (ActivityStopped)
            return;
        CloseFragment(settingsLangFragment);
        sendPacket_SetBusy(false);
        assertUserData();
    }

    public void CloseSettingsLang() {
        if (!ActivityStopped)
            doCloseSettingsLang();
        else
            postCloseSettingsLangFragment = true;
    }

    public void OpenSettingsTranslate() {
        if (FragmentStack.contains(settingsTranslateFragment) /*&& !settingsTranslateFragment.isHidden()*/)
            return;
        CloseSettingsFragments();
        if (user.isT) {
            settingsTranslateFragment.setTranslator(user);
            settingsTranslateFragment.FragmentOpened();
            sendPacket_SetBusy(true);
            OpenFragment(settingsTranslateFragment);
        }
    }

    protected void doCloseSettingsTranslate() {
        if (ActivityStopped)
            return;
        CloseFragment(settingsTranslateFragment);
        settingsTranslateFragment.FragmentClosed();
        sendPacket_SetBusy(false);
        assertUserData();
    }

    public void CloseSettingsTranslate() {
        if (!ActivityStopped)
            doCloseSettingsTranslate();
        else
            postCloseSettingsTranslateFragment = true;
    }

    public void OpenSettingsPhone() {
        if (FragmentStack.contains(settingsPhoneFragment)/* && !settingsPhoneFragment.isHidden()*/)
            return;
        CloseSettingsFragments();
        settingsPhoneFragment.setUser(user);
        if (!settingsPhoneFragment.AutoDeterminePhoneNumber()) {
            sendPacket_SetBusy(true);
            OpenFragment(settingsPhoneFragment);
        }
    }

    protected void doCloseSettingsPhone() {
        if (ActivityStopped)
            return;
        hideSoftKeyboard();
        CloseFragment(settingsPhoneFragment);
        sendPacket_SetBusy(false);
        assertUserData();
    }

    public void CloseSettingsPhone() {
        if (!ActivityStopped)
            doCloseSettingsPhone();
        else
            postCloseSettingsPhoneFragment = true;
    }

    public void CloseSettingsFragments() {
        CloseFragment(settingsUserFragment);
        if (user.isT) {
            CloseFragment(settingsTranslateFragment);
        } else
            CloseFragment(settingsLangFragment);
        CloseFragment(settingsPhoneFragment);
    }

    public void OpenHistory() {
        if (FragmentStack.contains(historyFragment)/* && !historyFragment.isHidden()*/)
            return;
        historyFragment.setUser(user);
        OpenFragment(historyFragment);
        historyFragment.FragmentOpened();
    }

    protected void doCloseHistory() {
        if (ActivityStopped)
            return;
//        hideSoftKeyboard();
        CloseFragment(historyFragment);
    }

    public void CloseHistory() {
        if (!ActivityStopped)
            doCloseHistory();
        else
            postCloseHistoryFragment = true;
    }

    public void OpenStatistic() {
        if (FragmentStack.contains(statisticFragment)/* && !historyFragment.isHidden()*/)
            return;
        if (Stat != null)
            statisticFragment.setStat(Stat);
        statisticFragment.FragmentOpened();
        OpenFragment(statisticFragment);
//        statisticFragment.FragmentOpened();
    }

    protected void doCloseStatistic() {
        if (ActivityStopped)
            return;
//        hideSoftKeyboard();
        CloseFragment(statisticFragment);
    }

    public void CloseStatistic() {
        if (!ActivityStopped)
            doCloseStatistic();
        else
            postCloseStatisticFragment = true;
    }

    public void OpenFragment(Fragment f) {
        if (f == null)
            return;
        if (!FragmentStack.empty()) {
            Fragment prev = FragmentStack.peek();
            SwapFragment(prev, f);
        } else
            SwapFragment(null, f);
        FragmentStack.push(f);
    }

    public void CloseFragment(Fragment f) {
        if (f == null)
            return;
        if (FragmentStack.isEmpty())
            return;
        if (FragmentStack.peek() != f)
            return;
        Fragment prevF = FragmentStack.pop();
        if (!FragmentStack.isEmpty())
            SwapFragment(prevF, FragmentStack.peek());
        else
            SwapFragment(prevF, null);
    }

    void SwapFragment(Fragment prevF, Fragment newF) {
        if (!ActivityCreated)
            return;
        if (prevF == tlistFragment)
            tlistFragment.FragmentHidden();

        FragmentTransaction ft = getSupportFragmentManager().beginTransaction();
        if (prevF != null)
            ft.hide(prevF);
//			ft.detach(prevF);
        if (!AddedFragment.contains(newF) && newF != null) {
            ft.add(R.id.frame_main, newF);
            AddedFragment.add(newF);
        }
        if (newF != null)
            ft.show(newF);
//			ft.attach(newF);
        ft.commit();
        if (newF == tlistFragment)
            tlistFragment.FragmentShown();
    }

    @Override
    protected void onResumeFragments() {
        // TODO Auto-generated method stub
        super.onResumeFragments();
        if (postOpenTListFragment) {
            doOpenPhonecallFragment();
            postOpenPhonecallFragment = false;
        }
        if (postOpenCListFragment) {
            doOpenCListFragment();
            postOpenCListFragment = false;
        }
        if (postOpenPhonecallFragment) {
            doOpenPhonecallFragment();
            postOpenPhonecallFragment = false;
        }
        if (postCloseSettingsUserFragment) {
            doCloseSettingsUser();
            postCloseSettingsUserFragment = false;
        }
        if (postClosePhonecallFragment) {
            doClosePhonecallFragment();
            postClosePhonecallFragment = false;
        }
        if (postCloseSettingsLangFragment) {
            doCloseSettingsLang();
            postCloseSettingsLangFragment = false;
        }
        if (postCloseSettingsTranslateFragment) {
            doCloseSettingsTranslate();
            postCloseSettingsTranslateFragment = false;
        }
        if (postCloseSettingsPhoneFragment) {
            doCloseSettingsPhone();
            postCloseSettingsPhoneFragment = false;
        }
        if (postCloseHistoryFragment) {
            doCloseHistory();
            postCloseHistoryFragment = false;
        }
        if (postCloseStatisticFragment) {
            doCloseStatistic();
            postCloseStatisticFragment = false;
        }
        if (postOpenMarkDialog) {
            doOpenMarkDialog(postedMarkRequest);
            postOpenMarkDialog = false;
        }
    }

    void doOpenMarkDialog(Parser.PacketMarkRequest MarkRequest) {
        if (MarkRequest == null || MarkRequest.translator == 0)
            return;
        RatingDialogFragment ratingFragment = new RatingDialogFragment();
//		ratingFragment.setFragment(PhonecallFragment.this);
        ratingFragment.init(this);
        User t = new User();
        t.id = MarkRequest.translator;
        t.name = MarkRequest.name;
        ratingFragment.setTranslator(t, MarkRequest.time);
        ratingFragment.show(getSupportFragmentManager(), "rating_dialog");
//		supportInvalidateOptionsMenu();
    }

    public void OpenMarkDialog(Parser.PacketMarkRequest p) {
        if (!ActivityStopped)
            doOpenMarkDialog(p);
        else {
            postedMarkRequest = p;
            postOpenMarkDialog = true;
        }
    }

    void UserMessage(String title, String msg) {
        if (isFinishing())
            return;
        AlertDialog.Builder dlgBuilder = new AlertDialog.Builder(MainActivity.this);
        dlgBuilder.setTitle(title);
        dlgBuilder.setMessage(msg);
        dlgBuilder.setCancelable(false);
        dlgBuilder.setPositiveButton(getString(R.string.ok), new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
                dialog.cancel();
            }
        });
        AlertDialog dlg1 = dlgBuilder.create();
        dlg1.show();
    }

    void UserMessageResetPassword(String title, String msg) {
        if (isFinishing())
            return;
        AlertDialog.Builder dlgBuilder = new AlertDialog.Builder(MainActivity.this);
        dlgBuilder.setTitle(title);
        dlgBuilder.setMessage(msg);
        dlgBuilder.setCancelable(false);
        dlgBuilder.setPositiveButton(getString(R.string.ok), new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
                sendPacket_ResetPassword();
                dialog.cancel();
            }
        });
        dlgBuilder.setNegativeButton(getString(R.string.cancel), new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
                dialog.cancel();
            }
        });
        AlertDialog dlg1 = dlgBuilder.create();
        dlg1.show();
    }

    void UserMessageSimple(String msg) {
        if (isFinishing())
            return;
        AlertDialog.Builder dlgBuilder = new AlertDialog.Builder(MainActivity.this);
        dlgBuilder.setMessage(msg);
        dlgBuilder.setCancelable(false);
        dlgBuilder.setPositiveButton("OK", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
                dialog.cancel();
            }
        });
        AlertDialog dlg1 = dlgBuilder.create();
        dlg1.show();
    }

    public static void EditTextSetError(EditText et, String msg) {
        SpannableStringBuilder ssbuilder = new SpannableStringBuilder(msg);
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.HONEYCOMB) {
            int ecolor = Color.RED; // whatever color you want
            ForegroundColorSpan fgcspan = new ForegroundColorSpan(ecolor);
            ssbuilder.setSpan(fgcspan, 0, msg.length(), 0);
        }
        et.setError(ssbuilder);
    }

    public static void TextViewSetError(TextView tv, String msg) {
        SpannableStringBuilder ssbuilder = new SpannableStringBuilder(msg);
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.HONEYCOMB) {
            int ecolor = Color.RED; // whatever color you want
            ForegroundColorSpan fgcspan = new ForegroundColorSpan(ecolor);
            ssbuilder.setSpan(fgcspan, 0, msg.length(), 0);
        }
        tv.setError(ssbuilder);
    }

    protected boolean LoadMenu(Menu menu) {
        if (user == null || FragmentStack.empty())// || FragmentStack.peek() == phonecallFragment)
            return false;
        if (user.isT)
            getMenuInflater().inflate(R.menu.main_translator, menu);
        else
            getMenuInflater().inflate(R.menu.main_client, menu);
        return super.onPrepareOptionsMenu(menu);
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        menu.clear();
        return LoadMenu(menu);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        menu.clear();
        // Inflate the menu; this adds items to the action bar if it is present.
        return LoadMenu(menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        if (clistFragment != null && FragmentStack.contains(clistFragment)) {
            if (clistFragment.isProcessingRequests()) {
                UserMessageSimple(getString(R.string.error_processing_active));
                return false;
            }
        }
        int id = item.getItemId();
        switch (id) {
            case R.id.action_settings_user:
                OpenSettingsUser();
                return true;
/*            case R.id.action_settings_lang:
        	    OpenSettingsLang();
                return true;
                */
            case R.id.action_settings_translate:
                OpenSettingsTranslate();
                return true;
            case R.id.action_settings_phone:
                OpenSettingsPhone();
                return true;
            case R.id.action_history:
                OpenHistory();
                return true;
            case R.id.action_statistics:
                OpenStatistic();
                return true;
        }
        return super.onOptionsItemSelected(item);
    }

    public void hideSoftKeyboard() {
        if (getCurrentFocus() != null) {
            InputMethodManager inputMethodManager = (InputMethodManager) getSystemService(INPUT_METHOD_SERVICE);
            inputMethodManager.hideSoftInputFromWindow(getCurrentFocus().getWindowToken(), 0);
        }
    }

    private View mFormView;
    private View mStatusView;
    private TextView mStatusMessageView;

    @TargetApi(Build.VERSION_CODES.HONEYCOMB_MR2)
    public void showProgress(final boolean show) {
        if (mStatusView == null)
            return;
        String msg;
        if (show)
            msg = "show";
        else
            msg = "hide";
        Toast toast = Toast.makeText(getApplicationContext(), msg, Toast.LENGTH_SHORT);
//        toast.show();
        // On Honeycomb MR2 we have the ViewPropertyAnimator APIs, which allow
        // for very easy animations. If available, use these APIs to fade-in
        // the progress spinner.
        if (false && Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB_MR2) {
            int shortAnimTime = getResources().getInteger(
                    android.R.integer.config_shortAnimTime);

//            mStatusView.setVisibility(show ? View.VISIBLE : View.GONE);
            mStatusView.setVisibility(View.VISIBLE);
            mStatusView.animate().setDuration(shortAnimTime)
                    .alpha(show ? 1 : 0)
                    .setListener(new AnimatorListenerAdapter() {
                        @Override
                        public void onAnimationEnd(Animator animation) {
                            mStatusView.setVisibility(show ? View.VISIBLE
                                    : View.GONE);
                        }
                    });

            mFormView.setVisibility(View.VISIBLE);
//            mFormView.setVisibility(show ? View.GONE : View.VISIBLE);
            mFormView.animate().setDuration(shortAnimTime)
                    .alpha(show ? 0 : 1)
                    .setListener(new AnimatorListenerAdapter() {
                        @Override
                        public void onAnimationEnd(Animator animation) {
                            mFormView.setVisibility(show ? View.GONE
                                    : View.VISIBLE);
                        }
                    });
        } else {
            // The ViewPropertyAnimator APIs are not available, so simply show
            // and hide the relevant UI components.
            mStatusView.setVisibility(show ? View.VISIBLE : View.GONE);
            mFormView.setVisibility(show ? View.GONE : View.VISIBLE);
        }
    }

    public void Close() {
//        WAS_LOGGEDIN = TryRelogin && (user != null && user.isT);
        if (user != null)
            WAS_LOGGEDIN = WAS_LOGGEDIN && user.isT;
//        user = null;  // user set to null in onCreate
//		if (Connected)
//			Disconnect();
        if (mNetworkBound) {
//            sendPacket_SetBusy(false);
            Connected = false;
            getApplicationContext().unbindService(mNClientConnection);
            mNetworkBound = false;
        }

        StopThreads();
        setResult(RESULT_OK);

        finish();
    }

    @Override
    protected void onResume() {
        // TODO Auto-generated method stub
        super.onResume();
        ActivityStopped = false;
    }

    @Override
    protected void onStop() {
        // TODO Auto-generated method stub
        super.onStop();
        ActivityStopped = true;
//		stopStatisticThread();
        if (FragmentStack.contains(tlistFragment))
            tlistFragment.ActivityOnStop();
    }

    @Override
    protected void onStart() {
        // TODO Auto-generated method stub
        super.onStart();
        ActivityStopped = false;
//		stopStatisticThread();
//		startStatisticThread();
        if (FragmentStack.contains(tlistFragment))
            tlistFragment.ActivityOnStart();
    }

    protected void CreateFragments() {
//    	statusFragment = new StatusFragment();
//    	statusFragment.init(this);
        settingsFragment = new SettingsFragment();
        settingsFragment.init(this);
        tlistFragment = new TListFragment();
        tlistFragment.init(this);
//		tlistFragment.setListLang(Country.getCountryLang(getCountry()));
        phonecallFragment = new PhonecallFragment();
        phonecallFragment.init(this);
        settingsUserFragment = new SettingsUserFragment();
        settingsUserFragment.init(this);
        settingsLangFragment = new SettingsLangFragment();
        settingsLangFragment.init(this);
        settingsTranslateFragment = new SettingsTranslateFragment();
        settingsTranslateFragment.init(this);
        settingsPhoneFragment = new SettingsPhoneFragment();
        settingsPhoneFragment.init(this);
        historyFragment = new HistoryFragment();
        historyFragment.init(this);
        statisticFragment = new StatisticFragment();
        statisticFragment.init(this);
//        historyFragment.init(this);
/*        getSupportFragmentManager().beginTransaction()
                .add(R.id.frame_status, statusFragment)
                .commit();		
*/
        clistFragment = new CListFragment();
        clistFragment.init(this);
    }

    void ConnectToBillingService() {
        if(!DEBUG_EMU)
        {
            Intent BillingServiceIntent = new Intent("com.android.vending.billing.InAppBillingService.BIND");
            BillingServiceIntent.setPackage("com.android.vending");
            if (!getApplicationContext().bindService(BillingServiceIntent, mBillingServiceCon, Context.BIND_AUTO_CREATE)) {
                Toast toast = Toast.makeText(getApplicationContext(), getString(R.string.error_billing), Toast.LENGTH_LONG);
                toast.show();
                Disconnect(false);
            }
        }
    }

    void DisconnectFromBillingService() {
        if (!DEBUG_EMU) {
            try {
                if (mBillingService != null)
                    getApplicationContext().unbindService(mBillingServiceCon);
                mBillingService = null;
            } catch (Exception ex) {
                UserMessage("Error",ex.getMessage());
            }
        }
    }


	@Override
    protected void onCreate(Bundle savedInstanceState) {
        ActivityCreated = true;
        if (Country.isCountry(curCountry))
            WAS_LOGGEDIN = true;
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        if (savedInstanceState == null) {
        }
        if (!NetworkClient.IS_LOGGEDIN)                 // for the case of death in background
            finish();
        user = null;

DEBUG_EMU=Build.FINGERPRINT.startsWith("generic");
        Connected = true;			// started by LoginActivity
        getApplicationContext().bindService(NetworkClient.getInstance(this), mNClientConnection, Context.BIND_AUTO_CREATE);
        BusyWhileLoad = true;
        showProgress(true);
        parser = new Parser();

        CreateFragments();

		registerReceiver(CallNotificationReceiver,  new IntentFilter(ACTION_NOTIF_CANCEL_ALERT));
        options.CallMinBalance = -1;
        options.CallTimeFree = 0;
		mFormView = findViewById(R.id.AM_Form);
		mStatusView = findViewById(R.id.AM_status);
		mStatusMessageView = (TextView)findViewById(R.id.AM_status_message);
		phoneContacts.LoadContacts(getContentResolver());
        actionBar = getSupportActionBar();
    }
	protected void StopThreads() {
//		stopStatisticThread();
		stopPhonecallListener();
		stopAlertThread();
		if (phonecallStatusTimer != null)
			phonecallStatusTimer.cancel();		
	}
    @Override
	protected void onDestroy() {
        ActivityCreated = false;
		// TODO Auto-generated method stub
		super.onDestroy();
//		onCallEnded(null, 0);
		StopThreads();
		UnregNetworkClient();
        DisconnectFromBillingService();
        if (mNetworkBound) {
            getApplicationContext().unbindService(mNClientConnection);
            mNetworkBound = false;
        }
		unregisterReceiver(CallNotificationReceiver);
	};

	public String getPhoneNumber() {
		TelephonyManager tMgr = (TelephonyManager)getSystemService(Context.TELEPHONY_SERVICE);
		String mPhoneNumber = tMgr.getLine1Number();
		return mPhoneNumber;		
	}
    public void onGotCurrentCountry(String country) {
        if (!Country.isCountry(country)) {
            if (user == null || user.isT) {
                Disconnect(false);
                return;
            }
            return;
        }
        boolean changed = true;
        if (country != null && curCountry != null && curCountry.equalsIgnoreCase(country))
            changed = false;
        curCountry = country;
        if (changed)
            NC_sendPacket_SetCountry();
    }

	public int InitSendCurrentCountry() {
        String c = null;
        try {
            final TelephonyManager tm = (TelephonyManager) getApplicationContext().getSystemService(Context.TELEPHONY_SERVICE);
            if (tm.getPhoneType() != TelephonyManager.PHONE_TYPE_CDMA) { // device is not 3G (would be unreliable)
                String networkCountry = tm.getNetworkCountryIso();
                if (Country.isCountry(networkCountry))
                    c = networkCountry.toLowerCase(Locale.US);
            }
        }
        catch (Exception e) { }
        if (curCountry == null && (c == null || !Country.isCountry(c))) {
            geoLocation = new TNGeoLocation(this);
            if (!geoLocation.isLocationServiceEnabled()) {
                UserMessage(getString(R.string.error), getErrorMessage(getApplicationContext(), Parser.ERROR_UNKNOWN_COUNTRY));
                if (user == null || user.isT)
                    Disconnect(false);
            }
            geoLocation.requestCurrentCountry();
            return 1;
        }
        else {
            if (c == null)
                onGotCurrentCountry(curCountry);
            else
                onGotCurrentCountry(c);
            return 0;
        }
	}
	public static String getCurrentLanguage() {
		String locale = Locale.getDefault().getLanguage();
		return Lang.parseLocale(locale);
	}
	public static boolean checkEmail(String email) {
		if (email == null || email.isEmpty())
			return false;
        for (int i=0; i<email.length(); i++)
            if (email.charAt(i) == ' ' || email.charAt(i) == '<' || email.charAt(i) == '>')
                return false;
		int i;
		for (i=0; i<email.length(); i++)
			if (email.charAt(i) == '@')
				break;
		if (i == email.length() || i == 0)
			return false;
		for (	; i<email.length(); i++)
			if (email.charAt(i) == '.')
				break;
		if (i >= email.length()-1)
			return false;
		return true;
	}
    public static String removeTrailingSpaces(String email) {
        for (int i=0; i<email.length(); i++) {
            if (email.charAt(i) == ' ') {
                if (email.length() > 1)
                    email = email.substring(1, email.length());
                i--;
            } else
                break;
        }
        for (int i=email.length()-1; i>0; i--) {
            if (email.charAt(i) == ' ') {
                if (email.length() > i)
                    email = email.substring(0, i);
            } else
                break;
        }
        return email;
    }

	public static final int PHONE_MIN = 6;
	public static final int PHONE_MAX = 15;
	public static boolean checkPhoneNumber(String phone) {
		if (phone == null || phone.length() < PHONE_MIN || phone.length() > PHONE_MAX)
			return false;
		if (phone.charAt(0) != '+')
			return false;
		for (int i = 1; i < phone.length(); i++) {
			if (phone.charAt(i) < '0' || phone.charAt(i) > '9')
				return false;
		}
        boolean allZero = true;
        for (int i=1; i<phone.length(); i++)
            if (phone.charAt(i) != '0')
                allZero = false;
        if (allZero)
            return false;
		return true;
	}
	public static String getErrorMessage(Context context, int code) {
        String msg;
		switch (code) {
        case Parser.ERROR_NOERROR:
            msg = context.getString(R.string.ERROR_NOERROR);
            break;
        case Parser.ERROR_OTHER:
            msg = context.getString(R.string.ERROR_OTHER);
            break;
        case Parser.ERROR_MAINTENANCE:
            msg = context.getString(R.string.ERROR_MAINTENANCE);
            break;
        case Parser.ERROR_FORMAT:
            msg = context.getString(R.string.ERROR_FORMAT);
            break;
        case Parser.ERROR_NO_USER:
            msg = context.getString(R.string.ERROR_NO_USER);
            break;
        case Parser.ERROR_USER_OFFLINE:
            msg = context.getString(R.string.ERROR_USER_OFFLINE);
            break;
        case Parser.ERROR_USER_ALREADY_EXIST:
            msg = context.getString(R.string.ERROR_USER_ALREADY_EXIST);
            break;
        case Parser.ERROR_ALREADY_LOGIN:
            msg = context.getString(R.string.ERROR_ALREADY_LOGIN);
            break;
        case Parser.ERROR_LOGIN_REQUIRED:
            msg = context.getString(R.string.ERROR_LOGIN_REQUIRED);
            break;
        case Parser.ERROR_WRONG_PASSWORD:
            msg = context.getString(R.string.ERROR_WRONG_PASSWORD);
            break;
        case Parser.ERROR_WRONG_SMSCODE:
            msg = context.getString(R.string.ERROR_WRONG_SMSCODE);
            break;
        case Parser.ERROR_NO_USERDATA:
            msg = context.getString(R.string.ERROR_NO_USERDATA);
            break;
        case Parser.ERROR_NO_PHONE:
            msg = context.getString(R.string.ERROR_NO_PHONE);
            break;
        case Parser.ERROR_PHONE_CHANGED:
            msg = context.getString(R.string.ERROR_PHONE_CHANGED);
            break;
        case Parser.ERROR_NO_LANG:
            msg = context.getString(R.string.ERROR_NO_LANG);
            break;
        case Parser.ERROR_PHONE_AWAITING:
            msg = context.getString(R.string.ERROR_PHONE_AWAITING);
            break;
        case Parser.ERROR_TEMP_BLOCKED:
            msg = context.getString(R.string.ERROR_TEMP_BLOCKED_1);
            msg += SMS_BlockDays;
            msg += context.getString(R.string.ERROR_TEMP_BLOCKED_2);
            break;
        case Parser.ERROR_UNKOWN_CALL:
            msg = context.getString(R.string.ERROR_UNKOWN_CALL);
            break;
        case Parser.ERROR_CALL_EXIST:
            msg = context.getString(R.string.ERROR_CALL_EXIST);
            break;
        case Parser.ERROR_CALL_STATE:
            msg = context.getString(R.string.ERROR_CALL_STATE);
            break;
        case Parser.ERROR_BALANCE: {
            msg = context.getString(R.string.ERROR_BALANCE_1);
            if (!isT) {
                msg += " " + context.getString(R.string.ERROR_BALANCE_2);
                int min = 1;
                int sec = 0;
                if (getOptions().CallMinBalance > 0) {
                    min = getOptions().CallMinBalance / 60;
                    sec = getOptions().CallMinBalance - min * 60;
                }
                msg += " " + min + ":" + String.format("%02d", sec);
                msg += " " + context.getString(R.string.ERROR_BALANCE_3);
            }
            break;
        }
        case Parser.ERROR_PHONECALL_ERROR:
            msg = context.getString(R.string.ERROR_PHONECALL_ERROR);
            break;
        case Parser.ERROR_PEER_DISCON:
            msg = context.getString(R.string.ERROR_PEER_DISCON);
            break;
        case Parser.ERROR_RATING_ERROR:
            msg = context.getString(R.string.ERROR_RATING_ERROR);
            break;
        case Parser.ERROR_PAYPAL_TRANSFER_ACTIVE:
            msg = context.getString(R.string.ERROR_PAYPAL_TRANSFER_ACTIVE);
            break;
        case Parser.ERROR_PURCHASE_SIGNATURE:
            msg = context.getString(R.string.ERROR_PURCHASE_SIGNATURE);
            break;
        case Parser.ERROR_ANOTHER_LOGIN:
            msg = context.getString(R.string.ERROR_ANOTHER_LOGIN);
            break;
        case Parser.ERROR_ANOTHER_PHONE:
            msg = context.getString(R.string.ERROR_ANOTHER_PHONE);
            break;
        case Parser.ERROR_UNKNOWN_COUNTRY:
            msg = context.getString(R.string.ERROR_UNKNOWN_COUNTRY);
            break;
        case Parser.ERROR_NAME_EXIST:
            msg = context.getString(R.string.ERROR_NAME_EXIST);
            break;
        default:
            msg = context.getString(R.string.ERROR_UNKNOWN);
            break;
		}
        if (code == Parser.ERROR_PHONE_CHANGED || code == Parser.ERROR_PEER_DISCON ||
                code == Parser.ERROR_WRONG_SMSCODE || code == Parser.ERROR_PURCHASE_SIGNATURE ||
                code == Parser.ERROR_WRONG_PASSWORD)
            return msg;
        return "(" + code + ")" + msg;
	}

    String Countries_EU[] = {"be", "bg", "cz", "dk", "de", "ee", "ie", "el", "es", "fr",
                             "hr", "it", "cy", "lv", "lt", "lu", "hu", "mt", "nl", "at",
                             "pl", "pt", "ro", "si", "sk", "fi", "se", "uk"};
	public int getCountryImageRes(String country) {
        if (Arrays.asList(Countries_EU).contains(country))
            country = "eu";
		int res = getResources().getIdentifier("flag_" + country, "drawable", "com.translatenet");
		return res;
	}
	public String getLangCodeByName(String name) {
		Field[] langs = R.string.class.getFields();
		for (Field f : langs) {
			String l = f.getName();
			int rid = getResources().getIdentifier(l, "string", "com.translatenet");
			if (rid == 0)
				continue;
			String lang = (String)getResources().getText(rid);
			if (lang.compareToIgnoreCase(name) == 0)
				return l;
		}
		String code = Lang.NativeToCode(name);
		if (code != null && code.length() > 0)
			return code;
		return Lang.LangToCode(name);
	}
	public String getLangNameByCode(String code) {
		int rid = getResources().getIdentifier(code, "string", "com.translatenet");
		if (rid != 0) {
			String lang = (String)getResources().getText(rid);
			if (lang != null && lang.length() > 0)
				return lang;
		}
		return Lang.CodeToLang(code);
	}
}
