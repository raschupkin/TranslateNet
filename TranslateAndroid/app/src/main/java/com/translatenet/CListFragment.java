package com.translatenet;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.Map;

import com.translatenet.TListFragment.TInfo;

import android.media.MediaPlayer;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.RatingBar;
import android.widget.RelativeLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.FrameLayout.LayoutParams;
import android.text.format.Time;


public class CListFragment extends Fragment {
	User translator;
	public void setTranslator(User _translator)	{	
		translator = _translator;
		if (getView() != null)
			updateTranslatorView();
	}
	TextView AwaitCallsTV; 
	class PhonecallRequest {
		Parser.PacketPhonecallRequest packet;
        User client;
		boolean confirmed;
		Time timeReq = new Time();
		Time timeResp = new Time();
	}
	class CInfo {
		PhonecallRequest req;
		RelativeLayout layout;
		TextView NameLabelTV;
		TextView NameTV;
		TextView TranslateLangLabelTV;
		TextView TranslateLangTV;
		TextView ClientLangLabelTV;
		TextView ClientLangTV;
		TextView MaxTimeLabelTV;
		TextView MaxTimeTV;
		TextView CountryLabelTV;
		TextView CountryTV;
		Button RejectButton;
		Button AcceptButton;
	};
	public boolean isProcessingRequests() {
		LinearLayout ll = (LinearLayout)getView().findViewById(R.id.CListLayout);
		if (ll == null)
			return false;
		return ll.getChildCount() > 0 && AwaitCallsTV == null;
	}
	public Call getCallData(int id) {
		LinearLayout ll = (LinearLayout)getView().findViewById(R.id.CListLayout);
		if (ll == null)
			return null;
		for (int j=0; j<ll.getChildCount(); j++) {
			CInfo cinfo = (CInfo)ll.getChildAt(j).getTag();
			if (cinfo == null) 
				continue;
			if (cinfo.req.client.id == id) {
				Call call = new Call();
				call.client = new User();
				call.client.name = cinfo.req.client.name;
				call.ClientLang = cinfo.req.client.client_lang;
				call.TranslateLang = cinfo.req.client.translate_lang;
				call.price = cinfo.req.client.price;
				call.translator = translator;
				return call;
			}
		}
		return null;
	}
	protected void onCancelButton(CInfo cinfo) {
		activity.sendPacket_PhonecallConfirm(cinfo.req.client.id, false);
		removeClientItem(cinfo.req.client.id);
	}
	protected void onConfirmButton(CInfo cinfo) {
		activity.sendPacket_PhonecallConfirm(cinfo.req.client.id, true);
//        activity.sendPacket_PhonecallConfirm(cinfo.req.packet.client, true);
		cinfo.RejectButton.setEnabled(false);
		cinfo.AcceptButton.setEnabled(false);
	}
    User LoadUserFromRequest(Parser.PacketPhonecallRequest p) {
        User client = new User();
        client.id = p.client;
        client.name = p.name;
        client.price = p.price;
        client.balance = p.balance;
        client.country = p.country;
        client.client_lang = p.client_lang;
        client.translate_lang = p.translate_lang;
        return client;
    }

	public boolean onPacketPhonecallRequest(Parser.PacketPhonecallRequest p) {
		if (p == null)
			return false;
		PhonecallRequest req = new PhonecallRequest();

		req.packet = p;
        req.client = LoadUserFromRequest(p);

		req.timeReq.setToNow();
		updateClientItem(req);
		return true;
	}
	public void onPacketPhonecallTimeout(Parser.PacketPhonecallTimeout p) {
		if (p == null)
			return;
		removeClientItem(p.client);
	}
	public boolean onPacketPhonecallError(int code, int client) {
		if (code != Parser.ERROR_NOERROR)
			removeClientItem(client);
        return true;
	}
    public void onPacket_ClientList(ArrayList<User> clist) {
        for (int i=0; i<clist.size(); i++) {
            User c = clist.get(i);
            if (c == null)
                continue;
            if (!c.delete) {
                PhonecallRequest req = new PhonecallRequest();
                req.client = c;
/*
		req.packet = p;
*/
                req.timeReq.setToNow();
                updateClientItem(req);
                if (!Lang.isLang(c.client_lang) || !Lang.isLang(c.translate_lang) || !Country.isCountry(c.country)) {
                    activity.sendPacket_PhonecallConfirm(c.id, false);
                    return;
                }
//                MediaPlayer mPlayer = MediaPlayer.create(activity, R.raw.call_request);   // in NetworkClient preprocessing
//                mPlayer.start();
                Toast toast = Toast.makeText(activity, getString(R.string.phonecall_request), Toast.LENGTH_LONG);
                toast.show();
            }
        }
    }
	public void removeClientItem(int client) {
        if (getView() == null)
            return;
		LinearLayout ll = (LinearLayout)getView().findViewById(R.id.CListLayout);
		if (ll == null)
			return;
        if (client <= 0)
            return;
		for (int j=0; j<ll.getChildCount(); j++) {
			CInfo ci = (CInfo)ll.getChildAt(j).getTag();
			if (ci == null || ci.req == null || ci.req.client == null)
				continue;
			if (ci.req.client.id == client)
				ll.removeViewAt(j);
		}
		if (ll.getChildCount() == 0)
			addAwaitCallsLabel();
	}
	protected void updateCListItem(int pos, PhonecallRequest req) {
        if (getView() == null)
            return;
		if (req == null || req.client == null)
			return;
		LinearLayout ll = (LinearLayout)getView().findViewById(R.id.CListLayout);
		if (ll == null)
			return;
		RelativeLayout layout = (RelativeLayout)ll.getChildAt(pos);
		if (layout == null || layout.getTag() == null)
			return;
		CInfo cinfo = (CInfo)layout.getTag();
		cinfo.NameTV.setText(req.client.name);
		cinfo.TranslateLangTV.setText(Lang.CodeToLang(req.client.translate_lang));
		cinfo.ClientLangTV.setText(Lang.CodeToLang(req.client.client_lang));
		cinfo.CountryTV.setText(Country.CodeToCountry(req.client.country));
		cinfo.MaxTimeTV.setText(Integer.toString(req.client.balance/req.client.price));
		cinfo.AcceptButton.setEnabled(!req.confirmed);
		cinfo.RejectButton.setEnabled(!req.confirmed);
	}
	protected void addClientItem(LinearLayout ll, int pos, PhonecallRequest req) {
		CInfo cinfo = createCListItem();
		if (cinfo == null)
			return;
		cinfo.req = req;
		ll.addView(cinfo.layout, pos);
		cinfo.layout.setTag(cinfo);
		updateCListItem(pos, req);

        int height = 0;
        for (int i=0; i<ll.getChildCount(); i++)
            height += ll.getChildAt(i).getHeight();
        ScrollView sv = (ScrollView)getView().findViewById(R.id.CListScrollView);
        sv.setMinimumHeight(height);            // sometimes ScrollView not shows content
	}
	protected void updateClientItem(PhonecallRequest req) {
		if (getView() == null)
			return;
		removeAwaitCallsLabel();
		LinearLayout ll = (LinearLayout)getView().findViewById(R.id.CListLayout);
		if (ll == null)
			return;
		int j;
		for (j=0; j<ll.getChildCount(); j++) {
			CInfo ci = (CInfo)ll.getChildAt(j).getTag();
			if (ci == null || ci.req == null)
				continue;
			if (ci.req.client.id == req.client.id) {
				ci.req = req;
				updateCListItem(j, req);
				return;
			}
		}
		addClientItem(ll, ll.getChildCount(), req);
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
	protected CInfo createCListItem() {
		CInfo cinfo = new CInfo();
		cinfo.layout = new RelativeLayout(getView().getContext());

		cinfo.NameLabelTV = new TextView(getView().getContext());
		cinfo.NameLabelTV.setId(generateId());
		RelativeLayout.LayoutParams lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
		cinfo.NameLabelTV.setLayoutParams(lp);
		cinfo.NameLabelTV.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
		cinfo.NameLabelTV.setPadding(padding_hor, padding_vert, 0, 0);
		cinfo.NameLabelTV.setText(getString(R.string.name));
		cinfo.layout.addView(cinfo.NameLabelTV);

		cinfo.NameTV = new TextView(getView().getContext());
		cinfo.NameTV.setId(generateId());
		lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
		lp.addRule(RelativeLayout.RIGHT_OF, cinfo.NameLabelTV.getId());
		cinfo.NameTV.setLayoutParams(lp);
		cinfo.NameTV.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
		cinfo.NameTV.setPadding(padding_hor, padding_vert, 0, 0);
		cinfo.layout.addView(cinfo.NameTV);

		cinfo.TranslateLangLabelTV = new TextView(getView().getContext());
		cinfo.TranslateLangLabelTV.setId(generateId());
		lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
		lp.addRule(RelativeLayout.BELOW, cinfo.NameTV.getId());
		cinfo.TranslateLangLabelTV.setLayoutParams(lp);
		cinfo.TranslateLangLabelTV.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
		cinfo.TranslateLangLabelTV.setText(getString(R.string.translate));
		cinfo.TranslateLangLabelTV.setPadding(padding_hor, padding_vert, 0, 0);
		cinfo.layout.addView(cinfo.TranslateLangLabelTV);

		cinfo.TranslateLangTV = new TextView(getView().getContext());
		cinfo.TranslateLangTV.setId(generateId());
		lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
		lp.addRule(RelativeLayout.BELOW, cinfo.NameTV.getId());
		lp.addRule(RelativeLayout.RIGHT_OF, cinfo.TranslateLangLabelTV.getId());
		cinfo.TranslateLangTV.setLayoutParams(lp);
		cinfo.TranslateLangTV.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
		cinfo.TranslateLangTV.setPadding(padding_hor, padding_vert, 0, 0);
		cinfo.layout.addView(cinfo.TranslateLangTV);

		cinfo.ClientLangLabelTV = new TextView(getView().getContext());
		cinfo.ClientLangLabelTV.setId(generateId());
		lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
		lp.addRule(RelativeLayout.BELOW, cinfo.TranslateLangTV.getId());
		cinfo.ClientLangLabelTV.setLayoutParams(lp);
		cinfo.ClientLangLabelTV.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
		cinfo.ClientLangLabelTV.setText(getString(R.string.client));
		cinfo.ClientLangLabelTV.setPadding(padding_hor, padding_vert, 0, 0);
		cinfo.layout.addView(cinfo.ClientLangLabelTV);

		cinfo.ClientLangTV = new TextView(getView().getContext());
		cinfo.ClientLangTV.setId(generateId());
		lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
		lp.addRule(RelativeLayout.BELOW, cinfo.TranslateLangTV.getId());
		lp.addRule(RelativeLayout.RIGHT_OF, cinfo.ClientLangLabelTV.getId());
		cinfo.ClientLangTV.setLayoutParams(lp);
		cinfo.ClientLangTV.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
		cinfo.ClientLangTV.setPadding(padding_hor, padding_vert, 0, 0);
		cinfo.layout.addView(cinfo.ClientLangTV);

		cinfo.CountryLabelTV = new TextView(getView().getContext());
		cinfo.CountryLabelTV.setId(generateId());
		lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
		lp.addRule(RelativeLayout.BELOW, cinfo.ClientLangLabelTV.getId());
		cinfo.CountryLabelTV.setLayoutParams(lp);
		cinfo.CountryLabelTV.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
		cinfo.CountryLabelTV.setText(getString(R.string.country));
		cinfo.CountryLabelTV.setPadding(padding_hor, padding_vert, 0, 0);
		cinfo.layout.addView(cinfo.CountryLabelTV);

		cinfo.CountryTV = new TextView(getView().getContext());
		cinfo.CountryTV.setId(generateId());
		lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
		lp.addRule(RelativeLayout.BELOW, cinfo.ClientLangLabelTV.getId());
		lp.addRule(RelativeLayout.RIGHT_OF, cinfo.CountryLabelTV.getId());
		cinfo.CountryTV.setLayoutParams(lp);
		cinfo.CountryTV.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
		cinfo.CountryTV.setPadding(padding_hor, padding_vert, 0, 0);
		cinfo.layout.addView(cinfo.CountryTV);

		cinfo.MaxTimeLabelTV = new TextView(getView().getContext());
		cinfo.MaxTimeLabelTV.setId(generateId());
		lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
		lp.addRule(RelativeLayout.BELOW, cinfo.CountryTV.getId());
		cinfo.MaxTimeLabelTV.setLayoutParams(lp);
		cinfo.MaxTimeLabelTV.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
		cinfo.MaxTimeLabelTV.setText(getString(R.string.max_time));
		cinfo.MaxTimeLabelTV.setPadding(padding_hor, padding_vert, 0, 0);
		cinfo.layout.addView(cinfo.MaxTimeLabelTV);

		cinfo.MaxTimeTV = new TextView(getView().getContext());
		cinfo.MaxTimeTV.setId(generateId());
		lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
		lp.addRule(RelativeLayout.BELOW, cinfo.CountryTV.getId());
		lp.addRule(RelativeLayout.RIGHT_OF, cinfo.MaxTimeLabelTV.getId());
		cinfo.MaxTimeTV.setLayoutParams(lp);
		cinfo.MaxTimeTV.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
		cinfo.MaxTimeTV.setText(getString(R.string.status));
		cinfo.MaxTimeTV.setPadding(padding_hor, padding_vert, 0, 0);
		cinfo.layout.addView(cinfo.MaxTimeTV);

		cinfo.AcceptButton = new Button(getView().getContext());
		cinfo.AcceptButton.setId(generateId());
		cinfo.AcceptButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				LinearLayout ll = (LinearLayout)getView().findViewById(R.id.CListLayout);
				if (ll == null)
					return;
				for (int i=0; i<ll.getChildCount(); i++) {
					CInfo cinfo = (CInfo)ll.getChildAt(i).getTag();
					if (cinfo == null)
						continue;
					if (cinfo.AcceptButton == v) {
						onConfirmButton(cinfo);
					}
				}
			}
		});
		cinfo.AcceptButton.setText(getString(R.string.accept));
		lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
		lp.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);
		lp.setMargins(5, 5, 5, 5);
		cinfo.AcceptButton.setLayoutParams(lp);
		cinfo.AcceptButton.setEnabled(true);
		cinfo.layout.addView(cinfo.AcceptButton);

		cinfo.RejectButton = new Button(getView().getContext());
		cinfo.RejectButton.setId(generateId());
		cinfo.RejectButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				LinearLayout ll = (LinearLayout)getView().findViewById(R.id.CListLayout);
				if (ll == null)
					return;
				for (int i=0; i<ll.getChildCount(); i++) {
					CInfo cinfo = (CInfo)ll.getChildAt(i).getTag();
					if (cinfo == null)
						continue;
					if (cinfo.RejectButton == v) {
						onCancelButton(cinfo);
					}
				}
			}
		});
		cinfo.RejectButton.setText(getString(R.string.reject));
		lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
		lp.addRule(RelativeLayout.LEFT_OF, cinfo.AcceptButton.getId());
		lp.setMargins(5, 5, 5, 5);
		cinfo.RejectButton.setLayoutParams(lp);
		cinfo.RejectButton.setEnabled(true);
		cinfo.layout.addView(cinfo.RejectButton);
		return cinfo;
	}
	void addAwaitCallsLabel() {
		LinearLayout ll = (LinearLayout)getView().findViewById(R.id.CListLayout);
		if (ll == null)
			return;
		if (ll.getChildCount() > 0)
			return;
		AwaitCallsTV = new TextView(getView().getContext());
		AwaitCallsTV.setId(generateId());
		AwaitCallsTV.setTextSize(TypedValue.COMPLEX_UNIT_SP, 18);
		AwaitCallsTV.setText(getString(R.string.await_calls));
		AwaitCallsTV.setPadding(padding_hor, padding_vert, 0, 0);
		ll.addView(AwaitCallsTV);		
	}
	void removeAwaitCallsLabel() {
		LinearLayout ll = (LinearLayout)getView().findViewById(R.id.CListLayout);
		if (ll == null)
			return;
		ll.removeView(AwaitCallsTV);
		AwaitCallsTV = null;
	}
	void updateTranslatorView() {
		if (translator == null || getView() == null)
			return;
		TextView NameTV = (TextView)getView().findViewById(R.id.CL_NameTextView);
		if (translator.name != null)
			NameTV.setText(translator.name);
        TextView TLangsTV = (TextView)getView().findViewById(R.id.CL_TLangsTextView);
        if (translator.translate != null) {
            String llist = getString(R.string.list_lang_list);
            Iterator it = translator.translate.entrySet().iterator();
            int i = 0;
            while (it.hasNext()) {
                Map.Entry<String, Integer> t = (Map.Entry<String, Integer>)it.next();
                String lang = t.getKey();
                if (Lang.isLang(lang)) {
                    if (i > 0)
                        llist += ", ";
                    llist += Lang.CodeToLang(lang);
                }
                i++;
            }
            if (i == 0)
                llist = getString(R.string.default_value);
            llist += ".";
            TLangsTV.setText(llist);
        }
        TextView RatingTV = (TextView)getView().findViewById(R.id.CL_RatingTextView);
		if (translator.rating_num != 0) {
			RatingTV.setText(String.format("%.01f", ((float)translator.rating)/20));
		} else
            RatingTV.setText("");
		RatingBar ratingBar = (RatingBar)getView().findViewById(R.id.CL_RatingBar);
		ratingBar.setRating(((float)translator.rating)/20);
		TextView RatingNumTV = (TextView)getView().findViewById(R.id.CL_RatingNumTextView);
		RatingNumTV.setText(String.format("(%d)", translator.rating_num));
		TextView BalanceTV = (TextView)getView().findViewById(R.id.CL_BalanceTextView);
		BalanceTV.setText(activity.FormatPrice(translator.balance));
		LinearLayout ll = (LinearLayout)getView().findViewById(R.id.CListLayout);
		if (ll != null)
			if (ll.getChildCount() == 0)
				addAwaitCallsLabel();
	}
    public void UpdateClients() {
        activity.sendPacket_RequestClientList();
    }
	@Override
	public void onStart() {
		// TODO Auto-generated method stub
		super.onStart();
		if (translator != null)
			updateTranslatorView();
	}
	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
			Bundle savedInstanceState) {
		View v = inflater.inflate(R.layout.fragment_clist, container, false);
		TextView NameLabelTV = (TextView)v.findViewById(R.id.CL_NameLabelTextView);
		NameLabelTV.setText(getString(R.string.name));
		TextView BalanceLabelTV = (TextView)v.findViewById(R.id.CL_BalanceLabelTextView);
		BalanceLabelTV.setText(getString(R.string.balance) + ": ");
/*    	Button SelectLangButton = (Button)v.findViewById(R.id.SelectLangButton);
    	SelectLangButton.setOnClickListener(new OnClickListener()  {
    		@Override
    		public void onClick(View v) {
    			if (activity == null)
    				return;
    		}
    	});
    */
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