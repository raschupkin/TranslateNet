package com.translatenet;

import android.content.SharedPreferences;
import android.os.Bundle;
import android.webkit.WebView.FindListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;
import android.support.v4.app.Fragment;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnClickListener;

public class ResetFragment extends Fragment {
	boolean ResetPressed;
	EditText mEmailView;
	String email;
	public boolean resetPassword() {
		// Check for a valid email address.
		View focusView = null;
        email = MainActivity.removeTrailingSpaces(email);
		if (email.length() == 0) {
            MainActivity.EditTextSetError(mEmailView, getString(R.string.error_field_required));
			focusView = mEmailView;
			focusView.requestFocus();
			return false;
		} else if (!MainActivity.checkEmail(email)) {
            MainActivity.EditTextSetError(mEmailView, getString(R.string.error_invalid_email));
			focusView = mEmailView;
			focusView.requestFocus();
			return false;
		}
		activity.sendPacket_ResetPassword(email);
		return true;
	}
	public void onPacket_Error(int code) {
		switch (code) {
		case Parser.ERROR_NOERROR:
			activity.closeResetFragment();
            activity.UserMessage(activity.getString(R.string.note), activity.getString(R.string.reset_done));
			break;
		case Parser.ERROR_NO_USER:
            MainActivity.EditTextSetError(mEmailView, MainActivity.getErrorMessage(activity.getApplicationContext(), code));
			mEmailView.requestFocus();
			break;
		default:
			Toast toast = Toast.makeText(activity.getApplicationContext(), 
					MainActivity.getErrorMessage(activity.getApplicationContext(), code), Toast.LENGTH_LONG);
			toast.show();
			break;
		}
	}
	public void onConnected() { 
		if (ResetPressed) {
			resetPassword();
			ResetPressed = false;
		}
	}
	@Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View v = inflater.inflate(R.layout.fragment_reset_password, container, false);
        SharedPreferences settings = activity.getSharedPreferences(MainActivity.PreferencesName, 0);
	    String LastEmail = settings.getString(MainActivity.Preference_LastEmail, "");
		mEmailView = (EditText)v.findViewById(R.id.RP_EmailEditText);
        if (mEmailView == null)
            return v;
		mEmailView.setText(LastEmail);

		Button resetButton = (Button)v.findViewById(R.id.RP_ResetButton);
        resetButton.setOnClickListener(new OnClickListener()  {
        	@Override
        	public void onClick(View v) {
        		email = mEmailView.getText().toString();
				if (activity.isConnected()) {
					ResetPressed = false;
//					resetPassword();
				} else {
					ResetPressed = true;
					activity.ResetConnectError(true);
                    activity.ConnectReset(email);
				}
        	}
        });

        return v;
	}
	LoginActivity activity;
	public void init(LoginActivity _activity) {
		activity = _activity;
	}
}
