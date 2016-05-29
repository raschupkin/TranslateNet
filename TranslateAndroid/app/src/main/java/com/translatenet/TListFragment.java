package com.translatenet;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.util.TypedValue;

import android.widget.RatingBar;
import android.os.Bundle;

import java.util.ArrayList;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.ImageView;
import android.widget.Toast;
import android.widget.FrameLayout.LayoutParams;
import com.translatenet.MainActivity.LangCallback;

public class TListFragment extends Fragment {
	public static int SORT_ORDER_RATING = 1;
	public int SortOrder= SORT_ORDER_RATING;
    TextView mNoTranslatorsTV;
	User client;
	public void setClient(User _client) {
		client = _client;
        if (ListLang == null) {
            /*
            String cur_country = activity.getCurrentCountry();
            SharedPreferences settings = activity.getSharedPreferences(client.email, 0);
            String saved_country = settings.getString(activity.Preference_Country, "");
            String saved_ListLang = settings.getString(activity.Preference_ListLang, "");
            if (saved_country == null || saved_country == "" || cur_country.compareToIgnoreCase(saved_country) != 0)
                InitListLang();
            else
                setListLang(AssertListLang(saved_ListLang));
            SharedPreferences.Editor editor = settings.edit();
            editor.putString(activity.Preference_Country, cur_country);
            editor.commit();
            */
            SharedPreferences settings = activity.getSharedPreferences(client.email, 0);
            String saved_ListLang = settings.getString(activity.Preference_ListLang, "");
            if (!Lang.isLang(saved_ListLang))
                setListLang(Lang.DEFAULT);
            else
                setListLang(saved_ListLang);
        } else
		    if (getView() != null)
			    updateClientView();
	}
	public User getClient()						{	return client;			}	
	public boolean setNativeLang(String lang) {
		if (ListLang.compareToIgnoreCase(lang) == 0) {
			activity.UserMessage(getString(R.string.error), getString(R.string.same_lang));
			return false;
		}
		client.lang = lang;
		activity.showProgress(true);
		activity.sendPacket_UserData(client);
		return true;
	}
    class CallState {
        boolean await;
        boolean confirmed;
        boolean rejected;
        String phone;
    }
//    HashMap<Integer, CallState> CallStates = new HashMap<Integer, CallState>();

	String ListLang;
	public boolean setListLang(String _ListLang) {
		if (client != null && client.lang != null && _ListLang.compareToIgnoreCase(client.lang) == 0) {
			activity.UserMessage(activity.getString(R.string.error), activity.getString(R.string.same_lang));
//            ListLang = Lang.DEFAULT;
            if (ListLang == null || (client != null && client.lang != null && ListLang.compareToIgnoreCase(client.lang) == 0))
                ListLang = Lang.DEFAULT;
            if (ListLang == null || (client != null && client.lang != null && ListLang.compareToIgnoreCase(client.lang) == 0))
                ListLang = Lang.DEFAULT2;
		} else
		    ListLang = _ListLang;
        SharedPreferences settings = activity.getSharedPreferences(client.email, 0);
        SharedPreferences.Editor editor = settings.edit();
        editor.putString(activity.Preference_ListLang, ListLang);

        editor.commit();
		if (getView() != null)
			updateClientView();
		return true;
	}
	public String getListLang()					{	return ListLang;		}
	protected void updateClientView() {
		if (getView() == null)
			return;
		if (client != null) {
			Button nativeLangButton = (Button)getView().findViewById(R.id.TList_NativeLangButton);
			if (nativeLangButton == null)
				return;
			nativeLangButton.setText(activity.getLangNameByCode(client.lang));
		}
		if (ListLang != null) {
			Button listLangButton = (Button)getView().findViewById(R.id.TList_ListLangButton);
			if (listLangButton == null)
				return;
			listLangButton.setText(activity.getLangNameByCode(ListLang));
		}
        if (client != null) {
            TextView balanceTV = (TextView)getView().findViewById(R.id.TList_BalanceTextView);
            if (balanceTV == null)
                return;
            balanceTV.setText(activity.FormatPrice(client.balance));
        }
	}
	class TInfo {
		User translator;
		String UserLang;

		RelativeLayout layout;
		TextView NameLabelTV;
		TextView NameTV;
		TextView statusLabelTV;
		TextView statusTV;
		TextView priceLabelTV;
		TextView priceTV;
		TextView countryLabelTV;
		TextView countryTV;
		ImageView countryIV;
		TextView ratingLabelTV;
		RatingBar ratingBar;
		TextView ratingNumTV;
		Button pcallReqButton;
		Button pcallButton;
		Button callButton;
	};
	public Call getCallData(int translator) {
        if (getView() == null)
            return null;
		LinearLayout ll = (LinearLayout)getView().findViewById(R.id.TList_ScrollViewLayout);
		if (ll == null)
			return null;
		for (int j=0; j<ll.getChildCount(); j++) {
			TInfo tinfo = (TInfo)ll.getChildAt(j).getTag();
			if (tinfo == null)
				continue;
			if (tinfo.translator.id == translator) {
				Call call = new Call();
				call.client = getClient();
				call.translator = tinfo.translator;
                call.translator.phone = tinfo.translator.phone;
				call.translator.name = tinfo.translator.name;
				call.ClientLang = client.lang;
				call.TranslateLang = getListLang();
				if (!tinfo.translator.translate.containsKey(ListLang))
					return null;
				call.price = tinfo.translator.translate.get(ListLang);
				return call;
			}
		}
		return null;
	}
	boolean RequestedTList = false;
	protected int doRequestTranslatorList() {
		RequestedTList = true;
		return activity.sendPacket_RequestTranslatorList(ListLang);
	}
	protected int SearchTranslators() {
		if (activity == null)
			return -1;
        if (RequestedTList)
            return -1;
		if (ListLang == null || !Lang.isLang(ListLang))
			return -1;
		activity.showProgress(true);
/*		if (RequestedTList)
			return activity.sendPacket_StopTranslatorList();
		else
*/			return doRequestTranslatorList();
	}
	public boolean onErrorStopTranslatorList(int code) {
		RequestedTList = false;
        ClearTranslators();
/*        if (isFragmentShown())
            if (code == 0 && ListLang != null && Lang.isLang(ListLang) && activity != null)
                return doRequestTranslatorList();
                */
		return true;
	}
    protected void updateTranslatorStatus(TInfo tinfo, boolean resetRequest) {
        if (tinfo == null || tinfo.translator == null)
            return;
        User t = tinfo.translator;
        tinfo.statusTV.setText(getStatusText(tinfo.translator));
/*        if (cs == null) {
            tinfo.pcallButton.setEnabled(false);
            if (tinfo.translator != null)
                tinfo.pcallReqButton.setEnabled(!tinfo.translator.busy);
        } else {
 */           tinfo.pcallButton.setEnabled(t.confirmed && !t.busy);
            tinfo.pcallReqButton.setEnabled((resetRequest || (!t.await && !t.confirmed && !t.rejected)) &&
                    !t.busy);
 //       }
    }

	// accept == false && reject == false - no answer(low balance)
	protected int SetConfirmed(int translator, boolean await, boolean accept, boolean reject, String phone, boolean resetRequest) {
        if (getView() == null) // activity destroying
            return -1;
		LinearLayout ll = (LinearLayout)getView().findViewById(R.id.TList_ScrollViewLayout);
		if (ll == null)
			return -1;

        int j;
		for (j=0; j<ll.getChildCount(); j++) {
			TInfo tinfo = (TInfo)ll.getChildAt(j).getTag();
			if (tinfo == null)
				continue;
			if (tinfo.translator.id == translator) {
		        if (isResumed() && accept != tinfo.translator.confirmed) {
		        	String msg = getString(R.string.translator) + " ";
		        	int ToastTime = Toast.LENGTH_LONG;
		        	if (accept) {
		        		msg += tinfo.translator.name + " " + getString(R.string.confirmed_request);
		        		ToastTime *= 1.5;
		        	} else if (!resetRequest) {
                        msg += tinfo.translator.name + " " + getString(R.string.rejected_request);
                    } else if (reject)
                        msg = getString(R.string.call_timeout);
					Toast toast = Toast.makeText(activity.getApplicationContext(), msg, ToastTime);
					toast.show();
		        }
                tinfo.translator.await = await;
                tinfo.translator.confirmed = accept;
                tinfo.translator.rejected = reject;
                updateTranslatorStatus(tinfo, /*cs,*/ resetRequest);
                break;
			}
		}

        if (j == ll.getChildCount())
            return -1;
        return j;
	}
	public void onPacket_TranslatorList(ArrayList<User> tlist) {
		for (int i=0; i<tlist.size(); i++) {
            User t = tlist.get(i);
            if (t == null)
                continue;
            updateTranslatorItem(t);
            SetConfirmed(t.id, t.await, t.confirmed, t.rejected, t.phone, t.error);
        }

		activity.showProgress(false);
        if (getView() == null)
            return;
		LinearLayout ll = (LinearLayout)getView().findViewById(R.id.TList_ScrollViewLayout);
		if (ll == null)
			return;
        ll.requestLayout();

        if (ll.getChildCount() == 0)
            mNoTranslatorsTV.setVisibility(View.VISIBLE);
        else
            mNoTranslatorsTV.setVisibility(View.GONE);
	}
	public int onPacketPhonecallConfirm(Parser.PacketPhonecallConfirm p) {
		if (p == null)
			return -1;
		int pos = SetConfirmed(p.translator, false, p.accept, !p.accept, p.phone, false);
		if (!isResumed() && MissedConfirm != null)
			MissedConfirm.add(p.translator);
        if (pos >= 0) {
            if (getView() == null)
                return 0;
            ScrollView scrollView = (ScrollView)getView().findViewById(R.id.TList_ScrollView);
            scrollView.scrollTo(0, pos);
        }
		return 0;
	}
	public void onPacketPhonecallTimeout(Parser.PacketPhonecallTimeout p) {
		if (p == null)
			return;
		SetConfirmed(p.translator, false, false, true, null, true);
        if (getView() == null)
            return;
		LinearLayout ll = (LinearLayout)getView().findViewById(R.id.TList_ScrollViewLayout);
		if (ll == null)
			return;
		ll.requestLayout();
	}
	public boolean onPacketErrorUserData(int code) {
        activity.showProgress(false);
		switch (code) {
		case Parser.ERROR_NOERROR:
			activity.setUser(client);
			updateClientView();
			UpdateTranslators();
			return true;
        case Parser.ERROR_NO_USERDATA:
            activity.assertUserData();
            return true;
		default:
			client = activity.getUser();
			activity.UserMessage(getString(R.string.error), activity.getErrorMessage(activity.getApplicationContext(), code));
			return true;
		}
	}
	public boolean onPacket_PhonecallError(int code, int translator) {
		switch (code) {
        case Parser.ERROR_BALANCE:
			SetConfirmed(translator, false, false, false, null, true);
//            UserMessageLowBalance();
//            return true;
            break;
        default:
            SetConfirmed(translator, false, false, false, null, false);
            break;
		}
        activity.UserMessage(getString(R.string.request), MainActivity.getErrorMessage(activity.getApplicationContext(), code));
//			SetConfirmed(translator, false, true, null, true);
//		if (TListConfirmed.contains(translator))
//			TListConfirmed.remove((Object)translator);
        if (getView() == null)  // activity destroying
            return true;
        LinearLayout ll = (LinearLayout) getView().findViewById(R.id.TList_ScrollViewLayout);
        if (ll == null)
            return true;
        ll.requestLayout();
        return true;
	}
	protected void onButtonPhonecallRequest(TInfo tinfo) {
        tinfo.translator.await = true;
        tinfo.translator.confirmed = false;
        tinfo.translator.rejected = false;
		tinfo.pcallButton.setEnabled(false);
		tinfo.pcallReqButton.setEnabled(false);
		tinfo.statusTV.setText(getStatusText(tinfo.translator));
		activity.sendPacket_PhonecallRequest(tinfo.translator.id, ListLang);
	}
//	Baresip sip = new Baresip();
	protected void onButtonCallRequest(TInfo tinfo) {
//		sip.mod_init();
	}

	public void onCallStarted(int translator) {
        if (getView() == null)
            return;
        LinearLayout ll = (LinearLayout)getView().findViewById(R.id.TList_ScrollViewLayout);
        if (ll == null)
            return;
        int j;
        for (j=0; j<ll.getChildCount(); j++) {
            TInfo ti = (TInfo) ll.getChildAt(j).getTag();
            if (ti == null || ti.translator == null)
                continue;
            if (ti.translator.id == translator) {
                updateTListItem(j, ti.translator);
                return;
            }
        }
	}
	ArrayList<Integer> MissedConfirm;
	protected void onButtonPhonecall(TInfo tinfo) {
//		MissedConfirm = new ArrayList<Integer>();
//		updateTranslatorItem(tinfo.translator);
		activity.OpenPhonecallFragment(getCallData(tinfo.translator.id), true);
	}
	protected String getStatusText(User t) {
        String status = "";


            if (t.await) {
                if (t.busy)
                    status = getString(R.string.busy) + "(";
                status += getString(R.string.await);
                if (t.busy)
                    status += ")";
            } else if (t.confirmed) {
                if (t.busy)
                    status = getString(R.string.busy) + "(";
                status += getString(R.string.confirmed_adjective);
                if (t.busy)
                    status += ")";
            } else if (t.rejected) {
                if (t.busy)
                    status = getString(R.string.busy) + "(";
                status += getString(R.string.rejected_adjective);
                if (t.busy)
                    status += ")";
            } else if (!t.busy)
                status = getString(R.string.available);
            else if (t.busy)
                status = getString(R.string.busy);
        return status;
	}
	protected void updateTListItem(int i, User translator) {
		if (translator == null)
			return;
        if (getView() == null)
            return;
		LinearLayout ll = (LinearLayout)getView().findViewById(R.id.TList_ScrollViewLayout);
		if (ll == null)
			return;
		RelativeLayout layout = (RelativeLayout)ll.getChildAt(i);
		if (layout == null || layout.getTag() == null)
			return;
		TInfo tinfo = (TInfo)layout.getTag();

        String old_phone = null;
        if (tinfo.translator != null)
            old_phone = tinfo.translator.phone;
        tinfo.translator = translator;
        if (tinfo.translator.phone == null)
            tinfo.translator.phone = old_phone;

		tinfo.NameTV.setText(translator.name);
		tinfo.priceTV.setText(getString(R.string.price));
        int price_list = translator.translate.get(ListLang);
        int price_client = translator.translate.get(client.lang);
		if (translator.translate.containsKey(ListLang))
			tinfo.priceTV.setText(activity.FormatPrice(price_list>price_client?price_list:price_client) + "/" + getString(R.string.min));
		tinfo.countryTV.setText(Country.CodeToCountry(translator.country));
        if (translator.country.compareToIgnoreCase(Country.UNKNOWN) != 0)
		    tinfo.countryIV.setImageResource(activity.getCountryImageRes(translator.country));
		if (translator.rating_num > 0)
			tinfo.ratingBar.setRating(((float)translator.rating)/20);
		tinfo.ratingNumTV.setText("(" + Integer.toString(translator.rating_num) + ")");
//        tinfo.statusTV.setText(getStatusText(translator));
/*        if (!




.containsKey(translator.id)) {
            updateTranslatorStatus(tinfo, null, true);
        } else {
            CallState cs = CallStates.get(translator.id);
*/            updateTranslatorStatus(tinfo, /*cs,*/ false);
 //       }
	}
	protected void addTranslatorItem(LinearLayout ll, int pos, User translator) {
		if (translator.translate.get(ListLang) == null)
			return;
		TInfo tinfo = createTListItem();
		if (tinfo == null)
			return;
		tinfo.translator = translator;
		ll.addView(tinfo.layout, pos);
		tinfo.layout.setTag(tinfo);
		updateTListItem(pos, translator);
	}
	protected void updateTranslatorItem(User translator) {
        if (getView() == null)
            return;
		LinearLayout ll = (LinearLayout)getView().findViewById(R.id.TList_ScrollViewLayout);
		if (ll == null)
			return;
		int j;
		for (j=0; j<ll.getChildCount(); j++) {
			TInfo ti = (TInfo)ll.getChildAt(j).getTag();
			if (ti == null || ti.translator == null)
				continue;
			if (ti.translator.id == translator.id) {
				if (!translator.delete) {
					updateTListItem(j, translator);
				} else {
					ll.removeViewAt(j);
//                    if (CallStates.containsKey(translator.id))
//                        CallStates.remove(translator.id);
					j--;
				}
				return;
			} else if (!translator.delete && translator.rating > ti.translator.rating) {
				addTranslatorItem(ll, j, translator);
				return;
			}
		}
		if (j == ll.getChildCount())
			addTranslatorItem(ll, j, translator);
	}
	public void ClearTranslators() {
        if (getView() == null)
            return;
		LinearLayout ll = (LinearLayout)getView().findViewById(R.id.TList_ScrollViewLayout);
		if (ll == null)
			return;
		ll.removeAllViews();
	}
	public void UpdateTranslators() {
        if (client == null)
            return;
        RequestedTList = false;
		ClearTranslators();
		SearchTranslators();
	}
	class UpdateNativeLangCallback implements LangCallback {
		public void LangSelected(String lang) {
			if (!Lang.isLang(lang))
				return;
			if (client != null && !client.lang.equalsIgnoreCase(lang))
				if (setNativeLang(lang))
					UpdateTranslators();
		}
	}
	class UpdateListLangCallback implements LangCallback {
		public void LangSelected(String lang) {
			if (!Lang.isLang(lang))
				return;
			if (!ListLang.equalsIgnoreCase(lang))
				if (setListLang(lang))
					UpdateTranslators();
		}
	}
	static int free_id = 1;
	// Returns a valid id that isn't in use
	public int generateId(){
	    View v = getView().findViewById(free_id);
	    while (v != null){
	        v = getView().findViewById(++free_id);
	    }
	    return free_id++;
	}
	private static int padding_vert = 5;
	private static int padding_hor = 5;
    private static int FontSize = 14;
	protected TInfo createTListItem() {
		TInfo tinfo = new TInfo();
		tinfo.layout = new RelativeLayout(getView().getContext());

		tinfo.NameLabelTV = new TextView(getView().getContext());
		tinfo.NameLabelTV.setId(generateId());
		RelativeLayout.LayoutParams lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
		tinfo.NameLabelTV.setLayoutParams(lp);
		tinfo.NameLabelTV.setTextSize(TypedValue.COMPLEX_UNIT_SP, FontSize);
//		tinfo.NameLabelTV.setPadding(padding_hor, padding_vert, 0, 0);
        tinfo.NameLabelTV.setPadding(padding_hor, 0, 0, 0);
		tinfo.NameLabelTV.setText(getString(R.string.name));
		tinfo.layout.addView(tinfo.NameLabelTV);

		tinfo.NameTV = new TextView(getView().getContext());
		tinfo.NameTV.setId(generateId());
		lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
		lp.addRule(RelativeLayout.RIGHT_OF, tinfo.NameLabelTV.getId());
		tinfo.NameTV.setLayoutParams(lp);
		tinfo.NameTV.setTextSize(TypedValue.COMPLEX_UNIT_SP, FontSize);
		tinfo.NameTV.setPadding(padding_hor, 0, 0, 0);
		tinfo.layout.addView(tinfo.NameTV);

		tinfo.statusLabelTV = new TextView(getView().getContext());
		tinfo.statusLabelTV.setId(generateId());
//		if (u.translate.get(ListLang) == null)
	//		return null;
		lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
		lp.addRule(RelativeLayout.BELOW, tinfo.NameTV.getId());
		tinfo.statusLabelTV.setLayoutParams(lp);
		tinfo.statusLabelTV.setTextSize(TypedValue.COMPLEX_UNIT_SP, FontSize);
		tinfo.statusLabelTV.setText(getString(R.string.status));
		tinfo.statusLabelTV.setPadding(padding_hor, 0, 0, 0);
		tinfo.layout.addView(tinfo.statusLabelTV);

		tinfo.statusTV = new TextView(getView().getContext());
		tinfo.statusTV.setId(generateId());
		lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
		lp.addRule(RelativeLayout.BELOW, tinfo.NameTV.getId());
		lp.addRule(RelativeLayout.RIGHT_OF, tinfo.statusLabelTV.getId());
		tinfo.statusTV.setLayoutParams(lp);
		tinfo.statusTV.setTextSize(TypedValue.COMPLEX_UNIT_SP, FontSize);
		tinfo.statusTV.setPadding(padding_hor, 0, 0, 0);
		tinfo.layout.addView(tinfo.statusTV);

		tinfo.priceLabelTV = new TextView(getView().getContext());
		tinfo.priceLabelTV.setId(generateId());
		lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
		lp.addRule(RelativeLayout.BELOW, tinfo.statusLabelTV.getId());
		tinfo.priceLabelTV.setLayoutParams(lp);
		tinfo.priceLabelTV.setTextSize(TypedValue.COMPLEX_UNIT_SP, FontSize);
		tinfo.priceLabelTV.setPadding(padding_hor, 0, 0, 0);
		tinfo.priceLabelTV.setText(getString(R.string.price));
		tinfo.layout.addView(tinfo.priceLabelTV);

		tinfo.priceTV = new TextView(getView().getContext());
		tinfo.priceTV.setId(generateId());
		lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
		lp.addRule(RelativeLayout.BELOW, tinfo.statusTV.getId());
		lp.addRule(RelativeLayout.RIGHT_OF, tinfo.priceLabelTV.getId());
		tinfo.priceTV.setLayoutParams(lp);
		tinfo.priceTV.setTextSize(TypedValue.COMPLEX_UNIT_SP, FontSize);
		tinfo.priceTV.setPadding(padding_hor, 0, 0, 0);
		tinfo.layout.addView(tinfo.priceTV);

		tinfo.countryLabelTV = new TextView(getView().getContext());
		tinfo.countryLabelTV.setId(generateId());
		lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
		lp.addRule(RelativeLayout.BELOW, tinfo.priceLabelTV.getId());
		tinfo.countryLabelTV.setLayoutParams(lp);
		tinfo.countryLabelTV.setTextSize(TypedValue.COMPLEX_UNIT_SP, FontSize);
		tinfo.countryLabelTV.setPadding(padding_hor, 0, 0, 0);
		tinfo.countryLabelTV.setText(getString(R.string.country));
		tinfo.layout.addView(tinfo.countryLabelTV);

		tinfo.countryTV = new TextView(getView().getContext());
		tinfo.countryTV.setId(generateId());
		lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
		lp.addRule(RelativeLayout.BELOW, tinfo.priceLabelTV.getId());
		lp.addRule(RelativeLayout.RIGHT_OF, tinfo.countryLabelTV.getId());
		tinfo.countryTV.setLayoutParams(lp);
		tinfo.countryTV.setTextSize(TypedValue.COMPLEX_UNIT_SP, FontSize);
		tinfo.countryTV.setPadding(padding_hor, 0, 0, 0);
		tinfo.layout.addView(tinfo.countryTV);

		tinfo.countryIV = new ImageView(getView().getContext());
		tinfo.countryIV.setId(generateId());
		lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
		lp.addRule(RelativeLayout.BELOW, tinfo.priceLabelTV.getId());
		lp.addRule(RelativeLayout.RIGHT_OF, tinfo.countryTV.getId());
		tinfo.countryIV.setLayoutParams(lp);
		tinfo.countryIV.setPadding(padding_hor, 0, 0, 0);
		tinfo.layout.addView(tinfo.countryIV);

		tinfo.ratingLabelTV = new TextView(getView().getContext());
		tinfo.ratingLabelTV.setId(generateId());
		lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
		lp.addRule(RelativeLayout.BELOW, tinfo.countryLabelTV.getId());
		lp.addRule(RelativeLayout.RIGHT_OF, tinfo.ratingLabelTV.getId());
		tinfo.ratingLabelTV.setLayoutParams(lp);
		tinfo.ratingLabelTV.setTextSize(TypedValue.COMPLEX_UNIT_SP, FontSize);
		tinfo.ratingLabelTV.setPadding(padding_hor, 0, 0, 0);
		tinfo.ratingLabelTV.setText(getString(R.string.rating));
		tinfo.layout.addView(tinfo.ratingLabelTV);

		tinfo.ratingBar = new RatingBar(getView().getContext(), null, android.R.attr.ratingBarStyleSmall);
		tinfo.ratingBar.setId(generateId());
		lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
		lp.addRule(RelativeLayout.BELOW, tinfo.countryLabelTV.getId());
		lp.addRule(RelativeLayout.RIGHT_OF, tinfo.ratingLabelTV.getId());
//        lp.setMargins(0, (int)(padding_vert*1.5), 0, 0);
		tinfo.ratingBar.setLayoutParams(lp);
		tinfo.ratingBar.setIsIndicator(true);
		tinfo.layout.addView(tinfo.ratingBar);

		tinfo.ratingNumTV = new TextView(getView().getContext());
		tinfo.ratingNumTV.setId(generateId());
		lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
		lp.addRule(RelativeLayout.BELOW, tinfo.countryLabelTV.getId());
		lp.addRule(RelativeLayout.RIGHT_OF, tinfo.ratingBar.getId());
		tinfo.ratingNumTV.setLayoutParams(lp);
		tinfo.layout.addView(tinfo.ratingNumTV);

		tinfo.pcallButton = new Button(getView().getContext());
		tinfo.pcallButton.setId(generateId());
		tinfo.pcallButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				LinearLayout ll = (LinearLayout)getView().findViewById(R.id.TList_ScrollViewLayout);
				if (ll == null)
					return;
				for (int i=0; i<ll.getChildCount(); i++) {
					TInfo tinfo = (TInfo)ll.getChildAt(i).getTag();
					if (tinfo == null)
						continue;
					if (tinfo.pcallButton == v) {
						onButtonPhonecall(tinfo);
					}
				}
			}
		});
		tinfo.pcallButton.setText(getString(R.string.pcall_verb));
		lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
		lp.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);
		lp.setMargins(5, 5, 5, 5);
		tinfo.pcallButton.setLayoutParams(lp);
		tinfo.pcallButton.setEnabled(false);
		tinfo.layout.addView(tinfo.pcallButton);

		tinfo.pcallReqButton = new Button(getView().getContext());
		tinfo.pcallReqButton.setId(generateId());
		tinfo.pcallReqButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				LinearLayout ll = (LinearLayout)getView().findViewById(R.id.TList_ScrollViewLayout);
				if (ll == null)
					return;
				for (int i=0; i<ll.getChildCount(); i++) {
					TInfo tinfo = (TInfo)ll.getChildAt(i).getTag();
					if (tinfo.pcallReqButton == v) {
						onButtonPhonecallRequest(tinfo);
					}
				}
			}
		});
		tinfo.pcallReqButton.setText(getString(R.string.pcall_req_verb));
		lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
		lp.addRule(RelativeLayout.LEFT_OF, tinfo.pcallButton.getId());
		lp.setMargins(5, 5, 5, 5);
		tinfo.pcallReqButton.setLayoutParams(lp);
		tinfo.layout.addView(tinfo.pcallReqButton);

/*
		tinfo.callButton = new Button(getView().getContext());
		tinfo.callButton.setId(generateId());
		tinfo.callButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				LinearLayout ll = (LinearLayout)getView().findViewById(R.id.TList_ScrollViewLayout);
				if (ll == null)
					return;
				for (int i=0; i<ll.getChildCount(); i++) {
					TInfo tinfo = (TInfo)ll.getChildAt(i).getTag();
					if (tinfo.callButton == v) {
						onButtonCallRequest(tinfo);
					}
				}
			}
		});
		tinfo.callButton.setText(getString(R.string.call));
		lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
		lp.addRule(RelativeLayout.BELOW, tinfo.pcallButton.getId());
		lp.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);
		lp.setMargins(5, 5, 5, 5);
		tinfo.callButton.setLayoutParams(lp);
		tinfo.layout.addView(tinfo.callButton);
*/		return tinfo;
	}
	protected void showConfirmedTranslators(ArrayList<Integer> list) {
        String ConfirmedList = "";
        for (int i=0; i<list.size(); i++) {
        	Call call = getCallData(list.get(i));
        	if (call == null)
        		continue;;
        	if (i != 0)
        		ConfirmedList += ", ";
        	ConfirmedList += call.translator.name;
        }
        if (ConfirmedList.length() > 0) {
			Toast toast = Toast.makeText(activity.getApplicationContext(), getString(R.string.translators_confirmed) +  ConfirmedList, Toast.LENGTH_LONG);
			toast.show();
        }
	}
    void CallFinished(String number) {
/*        Iterator it = CallStates.entrySet().iterator();
        while (it.hasNext()) {
            Map.Entry pair = (Map.Entry)it.next();
            CallState cs = (CallState)pair.getValue();
            if (cs == null || cs.phone == null)
                continue;
            if (cs.phone.equalsIgnoreCase(number)) {
                cs.rejected = false;
                cs.confirmed = false;
                cs.await = false;
            }
        }
        */
    }

    boolean fragmentShown;
    public boolean isFragmentShown() {
        return fragmentShown;
    }
	public void FragmentShown() {
		if (MissedConfirm != null) {
			showConfirmedTranslators(MissedConfirm);
			MissedConfirm = null;
		}
        fragmentShown = true;
	}
	public void FragmentHidden() {
		MissedConfirm = new ArrayList<Integer>();
        fragmentShown = false;
	}
/*	String getCurCountryListLang() {
		String country = activity.getCurrentCountry();
		if (country == null || country.compareToIgnoreCase(Country.UNKNOWN) == 0) 
			return Lang.DEFAULT;
		Country c = Country.getCountryByCode(country);
		if (c == null)
			return Lang.DEFAULT;
		return c.lang;
	}
	*/
    protected String AssertListLang(String llang) {
        if (client == null)
            return llang;
        if (llang.compareToIgnoreCase(client.lang) == 0)
            llang = Lang.DEFAULT;
        if (llang.compareToIgnoreCase(client.lang) == 0)
            llang = "es";
        if (llang.compareToIgnoreCase(client.lang) == 0)
            llang = "fr";
        return llang;
    }
    protected void InitListLang() {
//        setListLang(AssertListLang(getCurCountryListLang()));
    }

    void UserMessageLowBalance() {
        if (activity.isFinishing())
            return;
        AlertDialog.Builder dlgBuilder = new AlertDialog.Builder(activity);
        dlgBuilder.setMessage(getString(R.string.low_balance));
        dlgBuilder.setCancelable(false);
        dlgBuilder.setPositiveButton(getString(R.string.ok), new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
                dialog.cancel();
            }
        });
        dlgBuilder.setNeutralButton(getString(R.string.purchase_credits), new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
                activity.OpenSettingsUser();
            }
        });
        AlertDialog dlg1 = dlgBuilder.create();
        dlg1.show();
    }

    public void ActivityOnStart() {
        UpdateTranslators();
    }
    public void ActivityOnStop() {
        activity.sendPacket_StopTranslatorList();
    }

	@Override
	public void onStart() {
		super.onStart();
		updateClientView();
		FragmentShown();
	};
    @Override
    public void onStop() {
        // TODO Auto-generated method stub
        super.onStop();
        FragmentHidden();
    }
    void InitNoTranslatorsTV(View v) {
        mNoTranslatorsTV = (TextView)v.findViewById(R.id.TList_NoTranslators);
        String text = getString(R.string.no_translators);
        if (activity.getOptions().ActiveTSearch)
            text += "\n" + getString(R.string.active_tsearch);
        mNoTranslatorsTV.setText(text);
    }

	@Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View v = inflater.inflate(R.layout.fragment_tlist, container, false);
        RequestedTList = false;
        Button NativeLangButton = (Button)v.findViewById(R.id.TList_NativeLangButton);
        NativeLangButton.setOnClickListener(new OnClickListener()  {
        	@Override
        	public void onClick(View v) {
        		if (activity == null)
        			return;
        		activity.SelectLang(null, new UpdateNativeLangCallback(), false);
        	}
        });
        Button ListLangButton = (Button)v.findViewById(R.id.TList_ListLangButton);
        ListLangButton.setOnClickListener(new OnClickListener()  {
        	@Override
        	public void onClick(View v) {
        		if (activity == null)
        			return;
        		activity.SelectLang(null, new UpdateListLangCallback(), false);
        	}
        });
//        ListLang = getCurCountryListLang();
        InitNoTranslatorsTV(v);
        Button PurchaseCreditsButton = (Button)v.findViewById(R.id.TList_PurchaseCreditsButton);
        PurchaseCreditsButton.setOnClickListener(new OnClickListener()  {
            @Override
            public void onClick(View v) {
                if (activity == null)
                    return;
                activity.OpenSettingsUser();
            }
        });
        return v;
	}
	public void EnableMenu() {
		setHasOptionsMenu(true);
	}
	MainActivity activity;
	public void init(MainActivity _activity) {
		activity = _activity;
	}
}
