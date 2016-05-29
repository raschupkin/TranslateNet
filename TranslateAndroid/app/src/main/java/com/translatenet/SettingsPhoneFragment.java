package com.translatenet;

import android.telephony.TelephonyManager;
import android.content.Context;
import android.os.Bundle;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import java.util.Timer;
import java.util.TimerTask;

public class SettingsPhoneFragment extends Fragment {
    public static final int TIMER_BUTTON_RESEND_SMS = 5;
	User user;
	void setUser(User _user) {
		if (_user == null)
			return;
		try {
			user = (User)_user.clone();
		} catch (Exception ex) {
		}
		if (getView() != null)
			UpdateUserView();
	}
	void UpdateUserView() {
		if (getView() == null || user == null)
			return;
		TextView statusTV = (TextView)getView().findViewById(R.id.SP_StatusTextView);
		TextView phoneTV = (TextView)getView().findViewById(R.id.SP_PhoneTextView);
		TextView codeTV = (TextView)getView().findViewById(R.id.SP_CodeLabelTextView);
		EditText codeET = (EditText)getView().findViewById(R.id.SP_CodeEditText);
		Button enterButton = (Button)getView().findViewById(R.id.SP_DoneButton);
        Button resendSMSButton = (Button)getView().findViewById(R.id.SP_ResendSMSButton);
		switch (user.phone_status) {
		case User.PHONE_STATUS_NONE:
			statusTV.setText(getString(R.string.status_phone_none));
			phoneTV.setText(getString(R.string.default_value));
			phoneTV.setEnabled(true);
			codeTV.setEnabled(false);
			codeET.setEnabled(false);
			codeET.setText("");
			enterButton.setEnabled(false);
            resendSMSButton.setEnabled(false);
			break;
		case User.PHONE_STATUS_CONFIRMED:
			statusTV.setText(getString(R.string.status_phone_confirmed));
			phoneTV.setText(user.phone);
			phoneTV.setEnabled(true);
			codeTV.setEnabled(false);
			codeET.setEnabled(false);
			codeET.setText("");
			enterButton.setEnabled(false);
            resendSMSButton.setEnabled(false);
			break;
		case User.PHONE_STATUS_AWAIT:
			statusTV.setText(getString(R.string.status_phone_await));
			phoneTV.setText(user.await_phone);
			phoneTV.setEnabled(false);
			codeTV.setEnabled(true);
			codeET.setEnabled(true	);
			enterButton.setEnabled(true);
			if (EnteredPhone != null)
				phoneTV.setText(EnteredPhone);
			break;
		}
	}

    void NotifyUser_SMSSent() {
        Toast toast = Toast.makeText(activity.getApplicationContext(), getString(R.string.sms_sent), Toast.LENGTH_SHORT);
        toast.show();
    }

	public boolean onPacketError_RegisterPhone(Parser.PacketError p) {
        activity.showProgress(false);
        switch (p.code) {
            case Parser.ERROR_NOERROR:
                NotifyUser_SMSSent();
                startTimer_ButtonResendSMS();
                UpdateUserView();
                break;
            case Parser.ERROR_TEMP_BLOCKED:
                activity.SMS_BlockDays = p.sms_block_days;
                activity.SMS_SentNum = p.sms_sent_num;
                activity.UserMessage(getString(R.string.error), activity.getErrorMessage(activity.getApplicationContext(), p.code));
                break;
            default:
                activity.UserMessage(getString(R.string.error), getString(R.string.error_register_phone));
                break;
        }
        return true;
	}

    public void onPacket_AwaitPhoneConfirm() {
        activity.showProgress(false);
        user.phone_status = User.PHONE_STATUS_AWAIT;
        user.await_phone = EnteredPhone;
        activity.setUser(user);
        NotifyUser_SMSSent();
        startTimer_ButtonResendSMS();
        UpdateUserView();
    }
    public boolean onPacketError_ResendSMS(Parser.PacketError p) {
//		if (EnteredPhone == null)
//			return;
        startTimer_ButtonResendSMS();
        activity.showProgress(false);
        switch (p.code) {
            case Parser.ERROR_NOERROR:
                NotifyUser_SMSSent();
                return true;
            case Parser.ERROR_TEMP_BLOCKED:
            default:
                return onPacketError_RegisterPhone(p);
        }
    }

	String EnteredPhone;
	public void onEnteredPhonenumber(String phone) {
        if (getView() != null) {
            Button ResendSMSButton = (Button) getView().findViewById(R.id.SP_ResendSMSButton);
            if (ResendSMSButton != null)
                ResendSMSButton.setEnabled(false);
        }

        if (user.phone_status == User.PHONE_STATUS_CONFIRMED)
		    if (user.phone != null && phone.compareToIgnoreCase(user.phone) == 0)
			    return;
        else if (user.phone_status == User.PHONE_STATUS_AWAIT)
            if (user.await_phone != null && phone.compareToIgnoreCase(user.await_phone) == 0)
                return;
		EnteredPhone = phone;
		activity.showProgress(true);
		activity.sendPacket_RegisterPhone(phone, 1, GetDeviceID());
	}

	public boolean onPacketError_ConfirmRegisterPhone(Parser.PacketError p) {
//		if (EnteredPhone == null)
//			return;
		activity.showProgress(false);
		switch (p.code) {
		case Parser.ERROR_NOERROR:
            if (EnteredPhone != null) {
                user.phone = EnteredPhone;
                EnteredPhone = null;
            } else if (user.await_phone != null)
                user.phone = user.await_phone;
			user.phone_status = User.PHONE_STATUS_CONFIRMED;
			UpdateUserView();

			activity.setUser(user);
			activity.CloseSettingsPhone();
			Toast toast = Toast.makeText(activity.getApplicationContext(), getString(R.string.phone_success), Toast.LENGTH_LONG);
			toast.show();
			return true;
		default:
			user = activity.getUser();
			activity.UserMessage(getString(R.string.error), activity.getErrorMessage(activity.getApplicationContext(), p.code));
			return true;
		}
	}

	public void onEnteredCode(String code) {
//		if (EnteredPhone == null)
//			return;
		if (code == null || code.length() == 0)
			return;
		activity.sendPacket_ConfirmRegisterPhone(code);
	}
	public boolean checkCode(String code) {
		for (int i=0; i<code.length(); i++)
			if (code.charAt(i) < '0' || code.charAt(i) > '9')
				return false;
		return true;
	}
	
	public static class PhoneDialogFragment extends DialogFragment {
		SettingsPhoneFragment fragment;
		String phone;
		public void setFragment(SettingsPhoneFragment _fragment) {
			fragment = _fragment;
		}
		@Override
		public void onStart() {
			// TODO Auto-generated method stub
			super.onStart();
    		getDialog().setTitle(getString(R.string.settings_phone_descr));
		}
		@Override
		public View onCreateView(LayoutInflater inflater, ViewGroup container,
				Bundle savedInstanceState) {
			// TODO Auto-generated method stub
			View v = inflater.inflate(R.layout.fragment_settings_phone_dialog, container, false);
	        Button CancelButton = (Button)v.findViewById(R.id.SPH_CancelButton);
	        CancelButton.setOnClickListener(new OnClickListener()  {
	        	@Override
	        	public void onClick(View v) {
	        		dismiss();
	        	}
	        });
	        Button DoneButton = (Button)v.findViewById(R.id.SPH_DoneButton);
	        DoneButton.setOnClickListener(new OnClickListener()  {
	        	@Override
	        	public void onClick(View v) {
	        		EditText phoneET = (EditText)getView().findViewById(R.id.SPH_PhoneEditText);
	        		String phone = phoneET.getText().toString();
	        		if (!MainActivity.checkPhoneNumber(phone)) {
	        			if (phone != null && phone.length() > 1 && phone.charAt(0) != '+')
                            MainActivity.EditTextSetError(phoneET, getString(R.string.invalid_phone_plus));
	        			else
                            MainActivity.EditTextSetError(phoneET,getString(R.string.invalid_phone));
	        			return;
	        		}
                    if (fragment != null)
	        		    fragment.onEnteredPhonenumber(phone);
	        		dismiss();
	        	}
	        });
			return v;
		}
	}

	public String DeterminePhoneNumber() {
		TelephonyManager tMgr = (TelephonyManager)activity.getApplicationContext().getSystemService(Context.TELEPHONY_SERVICE);
		return tMgr.getLine1Number();
	}
	public String GetDeviceID() {
//		TelephonyManager tm = (TelephonyManager)activity.getApplicationContext().getSystemService(Context.TELEPHONY_SERVICE);
//		return tm.getDeviceId() + "_" + tm.getSimSerialNumber();
        return NetworkClient.DeviceID;
	}
	private boolean AutoDetermined = false;
	public boolean getAutoDetermined() {
		return AutoDetermined;
	}
	public boolean AutoDeterminePhoneNumber() {
		if (user == null)
			return false;
		String number = DeterminePhoneNumber();
//		number="0000001";
		if (number == null || number.length() == 0)
			return false;
        if (!MainActivity.checkPhoneNumber(number))
            return false;
		AutoDetermined = true;
		if (user.phone.compareToIgnoreCase(number) == 0) {
			Toast toast = Toast.makeText(activity.getApplicationContext(), getString(R.string.phone_msg) + user.phone, Toast.LENGTH_LONG);
			toast.show();
			return true;
		}
		activity.sendPacket_RegisterPhone(number, 0, GetDeviceID());
		return true;
	}

	@Override
	public void onStart() {
		// TODO Auto-generated method stub
		super.onStart();
		if (user != null)
			UpdateUserView();
	}
    Timer ButtonResendSMSTimer = null;
    void startTimer_ButtonResendSMS() {
        cancelTimer_ButtonResendSMS();
        ButtonResendSMSTimer = new Timer();
        int delay = TIMER_BUTTON_RESEND_SMS * 1000;
        ButtonResendSMSTimer.schedule(new TimerTask() {
            public void run() {
                activity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        if (getView() == null)
                            return;
                        Button ResendSMSButton = (Button) getView().findViewById(R.id.SP_ResendSMSButton);
                        if (ResendSMSButton != null)
                            ResendSMSButton.setEnabled(true);
                    }
                });
            }
        }, delay);
    }
    void cancelTimer_ButtonResendSMS(){
        if (ButtonResendSMSTimer != null)
            ButtonResendSMSTimer.cancel();
        ButtonResendSMSTimer = null;
    }

	@Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        cancelTimer_ButtonResendSMS();
        View v = inflater.inflate(R.layout.fragment_settings_phone, container, false);
        Button ChangeButton = (Button)v.findViewById(R.id.SP_ChangeButton);
        ChangeButton.setOnClickListener(new OnClickListener()  {
        	@Override
        	public void onClick(View v) {
       			PhoneDialogFragment phoneFragment = new PhoneDialogFragment();
       			phoneFragment.setFragment(SettingsPhoneFragment.this);
       			phoneFragment.show(getFragmentManager(), "phone_dialog");
        	}
        });
        Button ResendSMSButton = (Button)v.findViewById(R.id.SP_ResendSMSButton);
        ResendSMSButton.setOnClickListener(new OnClickListener()  {
            @Override
            public void onClick(View v) {
                if (user == null)
                    return;
                if (user.phone_status == User.PHONE_STATUS_AWAIT) {
                    activity.sendPacket_ResendSMS();
                    Button ResendSMSButton = (Button)v;
                    ResendSMSButton.setEnabled(false);
                    startTimer_ButtonResendSMS();
                }
            }
        });
        Button DoneButton = (Button)v.findViewById(R.id.SP_DoneButton);
        DoneButton.setOnClickListener(new OnClickListener()  {
        	@Override
        	public void onClick(View v) {
        		EditText codeTV = (EditText)getView().findViewById(R.id.SP_CodeEditText);
        		String code = codeTV.getText().toString();
        		if (!checkCode(code)) {
                    MainActivity.EditTextSetError(codeTV, getString(R.string.invalid_phone_code));
        			return;
        		}
        		onEnteredCode(code);
        	}
        });
		return v;
	}
	MainActivity activity;
	public MainActivity getMainActivity() {
		return activity;
	}
	public void init(MainActivity _activity) {
		activity = _activity;
	}
}
