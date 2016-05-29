package com.translatenet;

import java.util.Timer;
import java.util.TimerTask;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

public class LoginFragment extends Fragment {
	/**
	 * Keep track of the login task to ensure we can cancel it if requested.
	 */
	// Values for email and password at the time of the login attempt.
	private String mEmail;
	private String mPassword;
	// UI references.
	private EditText mEmailView;
	private EditText mPasswordView;
	final int MIN_PASSWORD = 3;
	final int MAX_PASSWORD = 8;
	boolean Loggedin;
	boolean LoginPressed;
	public int onPacket_LoginError(int code) {
		if (loginTimer != null)
			loginTimer.cancel();
		switch (code) {
		case Parser.ERROR_NO_PHONE:
		case Parser.ERROR_NOERROR:
        case Parser.ERROR_PHONE_AWAITING:
			Loggedin = true;
            showProgress(true);
		    SharedPreferences settings = activity.getSharedPreferences(MainActivity.PreferencesName, 0);
		    SharedPreferences.Editor editor = settings.edit();
		    editor.putString(MainActivity.Preference_LastEmail, mEmail);
		    editor.putString(MainActivity.Preference_LastPassword, mPassword);
		    editor.commit();
		    activity.sendPacket_SetBusy(true);
		    activity.sendPacket_GetLanguages();
		    return 0;
		case Parser.ERROR_NO_USER:
            MainActivity.EditTextSetError(mEmailView, MainActivity.getErrorMessage(activity.getApplicationContext(), code));
			mEmailView.requestFocus();
            showProgress(false);
			break;
		case Parser.ERROR_WRONG_PASSWORD:
            MainActivity.EditTextSetError(mPasswordView, MainActivity.getErrorMessage(activity.getApplicationContext(), code));
			mPasswordView.requestFocus();
            showProgress(false);
			break;
        case Parser.ERROR_PHONE_CHANGED:
            MainActivity.ReceivedError_PhoneChanged = true;
            break;
		default:
			Toast toast = Toast.makeText(activity.getApplicationContext(), 
					MainActivity.getErrorMessage(activity.getApplicationContext(), code), Toast.LENGTH_LONG);
			toast.show();
            showProgress(false);
            break;
		}
		return 0;
	}
	public int onError() {
		showProgress(false);
		return 0;
	}
	public String getCurrentCountry () {
		TelephonyManager tm = (TelephonyManager)activity.getSystemService(Context.TELEPHONY_SERVICE);
		String country = tm.getSimCountryIso();
		if (!Country.isCountry(country))
			country = Country.UNKNOWN;
		return country;
	}
	protected void sendLogin() {    // done in NetworkClient
/*		if (activity.request_sendPacket_Login(mEmail, mPassword, getCurrentCountry()) != 0) {
			Toast toast = Toast.makeText(activity.getApplicationContext(), getString(R.string.network_error_connect), Toast.LENGTH_LONG);
			toast.show();
		}
*/	}
	public void onConnected() {
		if (LoginPressed) {
//			sendLogin();
			LoginPressed = false;
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
						if (!activity.isActivityStopped()) {
//                            mEmailView.setError(getString(R.string.error_timeout));
                            MainActivity.EditTextSetError(mEmailView, getString(R.string.error_timeout));
                        }
					}
				});
				LoginPressed = false;
//				activity.Disconnect();
//				activity.Connect(false);
			}
		}, MainActivity.TIMEOUT_LOGIN);		
	}
    protected void stopTimeoutTimer() {
        if (loginTimer != null)
            loginTimer.cancel();
        loginTimer = null;
    }

	/**
	 * Attempts to sign in or register the account specified by the login form.
	 * If there are form errors (invalid email, missing fields, etc.), the
	 * errors are presented and no actual login attempt is made.
	 */
	public void attemptLogin() {
		// Reset errors.
		mEmailView.setError(null);
		mPasswordView.setError(null);

		// Store values at the time of the login attempt.
		mEmail = mEmailView.getText().toString();
		mPassword = mPasswordView.getText().toString();

		boolean cancel = false;
		View focusView = null;

		// Check for a valid password.
		if (TextUtils.isEmpty(mPassword)) {
            MainActivity.EditTextSetError(mPasswordView, getString(R.string.error_field_required));
			focusView = mPasswordView;
			cancel = true;
		} else if (mPassword.length() < MIN_PASSWORD) {
            MainActivity.EditTextSetError(mPasswordView, getString(R.string.error_short_password));
			focusView = mPasswordView;
			cancel = true;
		} else if (mPassword.length() < MIN_PASSWORD) {
            MainActivity.EditTextSetError(mPasswordView, getString(R.string.error_long_password));
			focusView = mPasswordView;
			cancel = true;
		}

        mEmail = MainActivity.removeTrailingSpaces(mEmail);
		// Check for a valid email address.
		if (TextUtils.isEmpty(mEmail)) {
            MainActivity.EditTextSetError(mEmailView, getString(R.string.error_field_required));
			focusView = mEmailView;
			cancel = true;
		} else if (!MainActivity.checkEmail(mEmail)) {
            MainActivity.EditTextSetError(mEmailView, getString(R.string.error_invalid_email));
			focusView = mEmailView;
			cancel = true;
		}

		if (cancel) {
			// There was an error; don't attempt login and focus the first
			// form field with an error.
			focusView.requestFocus();
		} else {
			// Show a progress spinner, and kick off a background task to
			// perform the user login attempt.
            activity.Disconnect();
            stopTimeoutTimer();
            createTimeoutTimer();
			if (activity.isConnected()) {
				sendLogin();                    // in NetworkClient now
				LoginPressed = false;
			} else {
				LoginPressed = true;
//				Toast toast = Toast.makeText(activity, getString(R.string.network_error_connect), Toast.LENGTH_SHORT);
//				toast.show();
				activity.ConnectLogin(mEmail, mPassword, getCurrentCountry());
                activity.hideSoftKeyboard();
                showProgress(true);
			}
		}
	}

	@Override
	public void onStop() {
		// TODO Auto-generated method stub
		super.onStop();
		LoginPressed = false;
	}
	void loadDefaultEmail() {
        if (activity == null)
            return;
        SharedPreferences settings = activity.getSharedPreferences(MainActivity.PreferencesName, 0);
	    String LastEmail = settings.getString(MainActivity.Preference_LastEmail, "");
	    String LastPassword = settings.getString(MainActivity.Preference_LastPassword, "");
		mEmailView.setText(LastEmail);
		mPasswordView.setText(LastPassword);
	}
    public void FragmentOpened() {
        loadDefaultEmail();
    }

	@Override
	public void onStart() {
		// TODO Auto-generated method stub
		super.onStart();
		Loggedin = false;
	}
	void showProgress(boolean show) {
		activity.showProgress(show, LoginActivity.PROGRESS_LOGIN);
	}
	@Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View v = inflater.inflate(R.layout.fragment_login, container, false);
		// Set up the login form.
//		mEmail = getIntent().getStringExtra(EXTRA_EMAIL);
		mEmailView = (EditText) v.findViewById(R.id.email);
//		mEmailView.setText(mEmail);
		mEmailView
		.setOnEditorActionListener(new TextView.OnEditorActionListener() {
			@Override
			public boolean onEditorAction(TextView textView, int id,
					KeyEvent keyEvent) {
				if (id == EditorInfo.IME_ACTION_DONE) {
					mPasswordView.requestFocus();
					return true;
				}
				return false;
			}
		});

		mPasswordView = (EditText) v.findViewById(R.id.password);
		mPasswordView
				.setOnEditorActionListener(new TextView.OnEditorActionListener() {
					@Override
					public boolean onEditorAction(TextView textView, int id,
							KeyEvent keyEvent) {
						if (id == EditorInfo.IME_ACTION_DONE) {
							attemptLogin();
							return true;
						}
						return false;
					}
				});


		v.findViewById(R.id.sign_in_button).setOnClickListener(
				new View.OnClickListener() {
					@Override
					public void onClick(View view) {
/*						if (Lang.Langs.size() == 0) {
							Toast toast;
							if (!activity.isConnected()) {
								activity.Disconnect();
								activity.Connect(true);
								toast = Toast.makeText(activity.getApplicationContext(), getString(R.string.connecting), Toast.LENGTH_SHORT);
							} else
								toast = Toast.makeText(activity.getApplicationContext(), getString(R.string.loading), Toast.LENGTH_SHORT);
							toast.show();
							return;
						}
*/						attemptLogin();
					}
				});       
		v.findViewById(R.id.register_button).setOnClickListener(
				new View.OnClickListener() {
					@Override
					public void onClick(View view) {
/*						if (Lang.Langs.size() == 0) {
							Toast toast;
							if (!activity.isConnected()) {
								activity.Disconnect();
								activity.Connect(true);
								toast = Toast.makeText(activity.getApplicationContext(), getString(R.string.connecting), Toast.LENGTH_SHORT);
							} else
								toast = Toast.makeText(activity.getApplicationContext(), getString(R.string.loading), Toast.LENGTH_SHORT);
							toast.show();
							return;
						}
*/						activity.openRegisterFragment();
					}
				});
        loadDefaultEmail();
		return v;
	}
	LoginActivity activity;
	public void init(LoginActivity _activity) {
		activity = _activity;
	}
}
