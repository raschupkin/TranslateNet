package com.translatenet;

import java.util.Timer;
import android.accounts.Account;
import android.accounts.AccountManager;
import java.util.List;
import java.util.LinkedList;
import java.util.TimerTask;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.annotation.TargetApi;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.os.Build;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.text.SpannableStringBuilder;
import android.text.style.ForegroundColorSpan;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.CheckBox;
import android.widget.Toast;

public class RegisterFragment extends Fragment {
	String mEmail;
	private EditText mEmailView;
	boolean RegisterPressed;
	public void onPacket_AwaitLoginConfirm() {
		showProgress(false);
        if (mEmail != null) {
            SharedPreferences settings = activity.getSharedPreferences(MainActivity.PreferencesName, 0);
            SharedPreferences.Editor editor = settings.edit();
            editor.putString(MainActivity.Preference_LastEmail, mEmail);
            editor.putString(MainActivity.Preference_LastPassword, "");
            editor.commit();
        }
        activity.Disconnect();
		activity.closeRegisterFragment();
		Toast toast = Toast.makeText(activity.getApplicationContext(), getString(R.string.user_created), Toast.LENGTH_LONG);
		toast.show();
//	    activity.StartMainActivity();
	}
	public void onPacket_Error(int code) {
        String msg = MainActivity.getErrorMessage(activity.getApplicationContext(), code);
        MainActivity.EditTextSetError(mEmailView, msg);
	}
	public int onError() {
		showProgress(false);
		return 0;
	}
	
	protected void sendRegister() {
		if (activity.sendPacket_RegisterUser(mEmail, isT) != 0) {
			Toast toast = Toast.makeText(activity.getApplicationContext(), getString(R.string.network_error_connect), Toast.LENGTH_LONG);
			toast.show();
		} else {
			showProgress(true);
			createTimeoutTimer();
		}
	}
	public void onConnected() { 
		if (RegisterPressed) {
			//sendRegister();
			RegisterPressed = false;
		}
	}

	Timer loginTimer;
	protected void createTimeoutTimer() {
		loginTimer = new Timer();
		loginTimer.schedule(new TimerTask() {
			public void run() {
				activity.runOnUiThread(new Runnable () {
					public void run() {
						showProgress(false);
						if (!activity.isActivityStopped())
							mEmailView.setError(getString(R.string.error_timeout));
					}
				});
				RegisterPressed = false;
				activity.Disconnect();
				activity.ResetConnectError(false);
			}
		}, MainActivity.TIMEOUT_LOGIN);		
	}
	boolean isT;
	@Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View v = inflater.inflate(R.layout.fragment_register, container, false);
		
        mEmailView = (EditText)v.findViewById(R.id.register_email);
        if (activity != null) {
            String email = MainActivity.getDefaultEmail(activity.getApplicationContext());
            if (email != null)
                mEmailView.setText(email);
        }
		
	    View.OnClickListener onClick = new View.OnClickListener() {	
			@Override
			public void onClick(View v) {
			    boolean checked = ((CheckBox) v).isChecked();
			    CheckBox clientCB = (CheckBox)getView().findViewById(R.id.register_client_checkbox);
			    CheckBox translatorCB = (CheckBox)getView().findViewById(R.id.register_translator_checkbox);
			    switch(v.getId()) {
			        case R.id.register_client_checkbox:
			        	translatorCB.setChecked(!checked);
			        	isT = !checked;
			            break;
			        case R.id.register_translator_checkbox:
			        	clientCB.setChecked(!checked);
			        	isT = checked;
			            break;
			    }
			}
		};
	    CheckBox clientCB = (CheckBox)v.findViewById(R.id.register_client_checkbox);
	    clientCB.setOnClickListener(onClick);
	    CheckBox translatorCB = (CheckBox)v.findViewById(R.id.register_translator_checkbox);
	    translatorCB.setOnClickListener(onClick);
	    
		v.findViewById(R.id.register_button).setOnClickListener(
				new View.OnClickListener() {
					@Override
					public void onClick(View view) {
						EditText emailET = (EditText)getView().findViewById(R.id.register_email);
						mEmail = emailET.getText().toString();
                        mEmail = MainActivity.removeTrailingSpaces(mEmail);
                        if (!MainActivity.checkEmail(mEmail)) {
                            emailET.setError(getString(R.string.invalid_email));
                            mEmail = null;
                            return;
                        }
						if (activity.isConnected()) {
							RegisterPressed = false;
							sendRegister();
						} else {
							RegisterPressed = true;
							activity.ResetConnectError(true);
                            activity.ConnectRegister(mEmail, isT);
							showProgress(true);
						}
					}
				});       
		v.findViewById(R.id.cancel_button).setOnClickListener(
				new View.OnClickListener() {
					@Override
					public void onClick(View view) {
						activity.closeRegisterFragment();
					}
				});       
		return v;
	}

	@Override
	public void onStop() {
		// TODO Auto-generated method stub
		super.onStop();
		RegisterPressed = false;
	}
	
	void showProgress(boolean show) {
		activity.showProgress(show, LoginActivity.PROGRESS_REGISTER);
	}
	boolean ShowProgress = false;
	public void setShowProgress(boolean show) {
		ShowProgress = show;
	}
	
	public void onStart() {
		super.onStart();
		if (ShowProgress == false)
			showProgress(ShowProgress);
	};

	LoginActivity activity;
	public void init(LoginActivity _activity) {
		activity = _activity;
	}
}
