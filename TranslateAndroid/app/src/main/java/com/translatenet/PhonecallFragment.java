package com.translatenet;
import java.util.Date;

import android.graphics.Color;
import android.os.Handler;
import java.util.Timer;
import java.util.TimerTask;
import android.os.Message;
import android.widget.Toast;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;

import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.TextView;


public class PhonecallFragment extends Fragment {
	Call cur_call = null;
	Date callStart = null;
	protected boolean MakingCall = false;
	protected boolean CallActive = false;
	protected boolean CallDialled = false;
	protected boolean AlertStarted;
	public boolean isCallActive() {
		return CallActive;
	}
	public boolean getMakingCall() {
		return MakingCall;
	}
	public void setMakingCall(boolean _makingCall) {
		MakingCall = _makingCall;
		EnableButtons();
	}
    protected boolean SameCall(Call call1, Call call2) {
        return (call1 == null || call1.client == null || call1.translator == null ||
                call2 == null || call2.client == null || call2.translator == null ||
                (call1.client.id == call2.client.id && call1.translator.id == call2.translator.id));
    }
	public void setCall(Call call) {
		if ((cur_call == null || !cur_call.active) && call != null) {
			if (call.active) {
				CallActive = true;
				CallDialled = true;
				EnableButtons();
			} else
				CallDialled = false;
		} else if (call == null || (!call.active && SameCall(call, cur_call))) {
			if (CallActive)
				CallFinished(cur_call.length);
			CallActive = false;
		}
		cur_call = call;
		if (CallActive && callStart == null && call == null) {
            callStart = new Date(System.currentTimeMillis());
        }
        if (call != null && call.length != 0) {
            callStart = new Date();
            callStart.setTime(System.currentTimeMillis()-call.length*1000);
            SetLengthTV(getView(), call.length);
        }
		if (getView() != null)
			updatePhonecallData();
	}
	public Call getCall() {
		return cur_call;
	}
	public void setAlertStarted() {
		AlertStarted = true;
		updatePhonecallData();
	}
	public boolean isAlertStarted() {
		return AlertStarted;
	}
	public void CallFinished(int time) {
		callStart = null;
        StopTimer();
        if (cur_call != null) {
            if (time > 0)
                cur_call.length = time;
            cur_call.active = false;
        }

        updatePhonecallData();
        CallActive = false;
        MakingCall = false;
        AlertStarted = false;
        EnableButtons();
        activity.sendPacket_GetUserData();
//		if (call.client == null)
//			activity.ClosePhonecallFragment();
	}

    protected void updateBalanceData() {
        if (getView() == null)
            return;
        if (cur_call == null)
            return;

        Button PurchaseCreditsButton = (Button)getView().findViewById(R.id.TList_PurchaseCreditsButton);
        TextView BalanceLabelTV = (TextView)getView().findViewById(R.id.PCall_BalanceLabelTextView);
        TextView BalanceTV = (TextView)getView().findViewById(R.id.PCall_BalanceTextView);
        if (MainActivity.isT) {
            if (BalanceLabelTV != null)
                BalanceLabelTV.setText(getString(R.string.client_balance));
            if (PurchaseCreditsButton != null)
                PurchaseCreditsButton.setVisibility(View.GONE);
            User client = cur_call.client;
            if (client != null) {
                if (BalanceTV != null)
                    BalanceTV.setText(activity.FormatPrice(client.balance));
                if (client.balance < cur_call.cost)
                    BalanceTV.setTextColor(Color.RED);
                else
                    BalanceTV.setTextColor(Color.BLACK);
            }
        } else {
            if (BalanceLabelTV != null)
                BalanceLabelTV.setText(getString(R.string.balance));
            if (PurchaseCreditsButton != null)
                PurchaseCreditsButton.setVisibility(View.VISIBLE);
            User client = cur_call.client;
            if (client != null) {
                if (BalanceTV != null)
                    BalanceTV.setText(activity.FormatPrice(client.balance));
            }
        }
    }


	protected void updatePhonecallData() {
		if (getView() == null)
            return;
        if (cur_call == null)
			return;

        updateBalanceData();

//        if (!MakingCall && cur_call.length == 0 && !AlertStarted)
//            return;
		TextView statusTV = (TextView)getView().findViewById(R.id.PCall_StatusTextView);
		if (cur_call.active) {
			statusTV.setText(getString(R.string.active));
		} else {
			statusTV.setText(getString(R.string.idle));
//			TextView lengthTV = (TextView)getView().findViewById(R.id.PCall_LengthTextView);
//			lengthTV.setText(MainActivity.FormatTime(call.length));
		}
		TextView descrTV = (TextView)getView().findViewById(R.id.PCall_ConfirmedLabelTextView);
		if (MakingCall)
			descrTV.setText(getString(R.string.phonecall_to_translator));
		else if (cur_call.active && cur_call.client != null)
			descrTV.setText(getString(R.string.phonecall_from_client));
		else if (AlertStarted)
			descrTV.setText(getString(R.string.phonecall_unknown));
        else
            return;

		if (MakingCall || CallActive) {   // ^
			TextView nameTV = (TextView)getView().findViewById(R.id.PCall_NameTextView);
			if (MakingCall) {
				if (cur_call.translator != null && cur_call.translator.name != null)
					nameTV.setText(cur_call.translator.name);
			} else if (cur_call.client != null && cur_call.client.name != null)
				nameTV.setText(cur_call.client.name);
			TextView clientLangTV = (TextView)getView().findViewById(R.id.PCall_ClientLangTextView);
            if (cur_call.ClientLang != null)
			    clientLangTV.setText(Lang.CodeToLang(cur_call.ClientLang));
			TextView translateLangTV = (TextView)getView().findViewById(R.id.PCall_TranslateLangTextView);
            if (cur_call.TranslateLang != null)
			    translateLangTV.setText(Lang.CodeToLang(cur_call.TranslateLang));
            TextView priceTV = (TextView) getView().findViewById(R.id.PCall_PriceTextView);
            priceTV.setText(activity.FormatPrice(cur_call.price)+"/"+getString(R.string.min));
            TextView costTV = (TextView) getView().findViewById(R.id.PCall_CostTextView);
            costTV.setText(activity.FormatPrice(cur_call.cost));
        }
		EnableButtons();
	}
	protected void EnableButtons() { 
		if (getView() == null || cur_call == null)
			return;
		Button CallButton = (Button)getView().findViewById(R.id.PCall_CallButton);
		if (CallButton != null) {
			CallButton.setEnabled(MakingCall && !CallActive && !CallDialled);
//			CallButton.setVisibility(View.GONE);
		}
		Button CancelButton = (Button)getView().findViewById(R.id.PCall_CancelButton);
		if (CancelButton != null) {
			CancelButton.setEnabled(!CallActive);
//			CancelButton.setVisibility(View.GONE);
		}
		Button CancelAlertButton = (Button)getView().findViewById(R.id.PCall_CancelAlertButton);
		if (CancelAlertButton != null) {
			if (MainActivity.isT)
				CancelAlertButton.setVisibility(View.VISIBLE);
			else
				CancelAlertButton.setVisibility(View.GONE);
			CancelAlertButton.setEnabled(AlertStarted);
		}
	}
	public boolean onError_Phonecall(int code, int peer) {
		if (cur_call == null)
			return false;
		if (MakingCall) {
			if (cur_call.translator != null && peer == cur_call.translator.id) {
				Toast toast = Toast.makeText(getActivity(), activity.getErrorMessage(getActivity().getApplicationContext(), code), Toast.LENGTH_LONG);
				toast.show();
				activity.ClosePhonecallFragment();
			}
		} else if (cur_call.client != null && peer == cur_call.client.id){ 
			Toast toast = Toast.makeText(getActivity(), activity.getErrorMessage(getActivity().getApplicationContext(), code), Toast.LENGTH_LONG);
			toast.show();			
			activity.ClosePhonecallFragment();
		}
        return true;
	}
	@Override
	public void onStart() {
		// TODO Auto-generated method stub
		super.onStart();
		if (cur_call != null)
			updatePhonecallData();
	}
	@Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View v = inflater.inflate(R.layout.fragment_phonecall, container, false);
		TextView nameLabelTV = (TextView)v.findViewById(R.id.PCall_NameLabelTextView);
		nameLabelTV.setText(getString(R.string.name));
		Button CancelButton = (Button)v.findViewById(R.id.PCall_CancelButton);
		CancelButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                if (activity == null)
                    return;
                StopTimer();
                activity.ClosePhonecallFragment();
            }
        });
		Button CallButton = (Button)v.findViewById(R.id.PCall_CallButton);
		CallButton.setOnClickListener(new OnClickListener()  {
			@Override
			public void onClick(View v) {
				if (activity == null)
					return;
				if (cur_call == null || cur_call.translator == null || cur_call.translator.phone == null)
					return;
				String url = "tel:";
				url += cur_call.translator.phone;
				startActivity(new Intent(Intent.ACTION_CALL, Uri.parse(url)));
			}
		});
		Button CancelAlertButton = (Button)v.findViewById(R.id.PCall_CancelAlertButton);
		CancelAlertButton.setOnClickListener(new OnClickListener()  {
			@Override
			public void onClick(View v) {
				doCancelAlert();
			}
		});
        Button PurchaseCreditsButton = (Button)v.findViewById(R.id.PCall_PurchaseCreditsButton);
        PurchaseCreditsButton.setOnClickListener(new OnClickListener()  {
            @Override
            public void onClick(View v) {
                activity.OpenSettingsUser();
            }
        });
        SetLengthTV(v, 0);
//		callTimer.schedule(callTimeTask, 0, 1000);
        return v;
	}
	@Override
	public void onDestroyView() {
		// TODO Auto-generated method stub
        StopTimer();
		super.onDestroyView();
	}
    protected void SetLengthTV(View mainView, long length) {
        if (mainView == null)
            return;
        TextView lengthTV = (TextView)mainView.findViewById(R.id.PCall_LengthTextView);
        lengthTV.setText(activity.FormatTime((int)length));
    }
    TimerTask callTimeTask;
    Timer callTimer = null;
    public Handler mTimeHandler = new Handler() {
        public void handleMessage(Message msg) {
            if (callStart == null)
                return;
            Date now = new Date(System.currentTimeMillis());
            long length = (now.getTime() - callStart.getTime())/1000;
            SetLengthTV(getView(), length);
        }
    };
    void StopTimer() {
        if (callTimer == null)
            return;
        callTimer.cancel();
        callTimer = null;
    }
    void StartTimer() {
        SetLengthTV(getView(), 0);
        StopTimer();
        callTimer = new Timer();
        callTimeTask = new TimerTask() {
            public void run() {
//				if (callStart == null)
//					return;
                mTimeHandler.obtainMessage(1).sendToTarget();
            }
        };
        callTimer.schedule(callTimeTask, 0, 1000);
    }
    public void doCancelAlert() {
        if (activity == null)
            return;
        AlertStarted = false;
        activity.CancelAlert();
        updatePhonecallData();
    }
    public void FragmentOpened() {
        EnableButtons();
        StartTimer();
    }
    void FragmentClosed() {
//        callStart = null;
        CallFinished(0);
        StopTimer();
    }

	MainActivity activity = null;
	public void init(MainActivity act) {
		activity = act;
	}
}
