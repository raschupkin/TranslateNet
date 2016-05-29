package com.translatenet;
import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.annotation.TargetApi;
import android.os.Build;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import com.translatenet.Lang;
import com.translatenet.MainActivity.LangCallback;
import com.translatenet.TListFragment.UpdateListLangCallback;

public class SettingsLangFragment extends Fragment {
	User client;
	public void setClient(User _client) {
		if (_client == null)
			return;
		try {
			client = (User)_client.clone();
		} catch (Exception ex) {
		}
		if (client.lang == null)
			client.lang = activity.getCurrentLanguage();
		UpdateLangView(activity.getLangNameByCode(client.lang));
	}
	public boolean onPacketErrorUserData(int code) {
		activity.showProgress(false);
		switch (code) {
		case Parser.ERROR_NOERROR:
    		activity.setUser(client);
    		activity.CloseSettingsLang();
//    		Toast toast = Toast.makeText(activity.getApplicationContext(), getString(R.string.lang_success), Toast.LENGTH_LONG);
//    		toast.show();
    		return true;
		default:
			setClient(activity.getUser());
			activity.UserMessage(getString(R.string.error), activity.getErrorMessage(activity.getApplicationContext(), code));
            return true;
		}
 	}
	void UpdateLangView(String langName) {
		if (getView() == null)
			return;
		TextView langTV = (TextView)getView().findViewById(R.id.SL_LangTextView);
		if (langTV != null)
			langTV.setText(langName);
	}
	class UpdateLangCallback implements LangCallback { 
		public void LangSelected(String lang) {
			if (!Lang.isLang(lang))
				return;
			UpdateLangView(activity.getLangNameByCode(lang));
			if (client != null)
				client.lang = lang; 
		}		
	}

	@Override
	public void onStart() {
		// TODO Auto-generated method stub
		super.onStart();
		if (client != null && client.lang != null)
			UpdateLangView(activity.getLangNameByCode(client.lang));
	}
	
	@Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View v = inflater.inflate(R.layout.fragment_settings_lang, container, false);
        Button SelectLangButton = (Button)v.findViewById(R.id.SL_SelectButton);
        SelectLangButton.setOnClickListener(new OnClickListener()  {
        	@Override
        	public void onClick(View v) {
        		if (activity == null)
        			return;
        		activity.SelectLang(null, new UpdateLangCallback(), false);
        	}
        });
        Button DoneButton = (Button)v.findViewById(R.id.SL_DoneButton);
        DoneButton.setOnClickListener(new OnClickListener()  {
        	@Override
        	public void onClick(View v) {
        		if (activity == null)
        			return;
        		if (client == null || client.lang == null || activity.getUser() == null )
        			return;
        		if (!client.lang.equalsIgnoreCase(activity.getUser().lang)) {
        			activity.showProgress(true);
        			activity.sendPacket_UserData(client);
        		} else {
        			activity.CloseSettingsLang();
        		}
        	}
        });
		return v;
	}
	MainActivity activity;
	public void init(MainActivity _activity) {
		activity = _activity;
	}
}
