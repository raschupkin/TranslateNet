package com.translatenet;

import android.os.Bundle;

import com.translatenet.Parser.PacketStatistic;

import android.support.v7.app.ActionBar;
import android.widget.Button;
import android.widget.TextView;
import android.view.View.OnClickListener;

import com.translatenet.MainActivity;

import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

public class StatusFragment extends Fragment {
    ActionBar actionBar;
    public void setActionBar(ActionBar ab) {
        actionBar = ab;
    }

	@Override
	public void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
	}
	@Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
/*        View v = inflater.inflate(R.layout.fragment_status, container, false);
		Button b = (Button)v.findViewById(R.id.optionsButton);
		b.setOnClickListener(new OnClickListener()  {
			@Override
			public void onClick(View v) {
				if (activity != null)
					activity.openOptionsMenu();
			}
		});
        return v;
        */
        return super.onCreateView(inflater, container, savedInstanceState);
    }
	public void onActivityCreated(Bundle savedInstanceState) {
		super.onActivityCreated(savedInstanceState);
		updateView(null);		
	}
	boolean loaded;
	Parser.PacketStatistic stat;
	protected void updateView(User user) {
/*		if (getView() == null)
			return;
		TextView tv = (TextView)getView().findViewById(R.id.statusTextView);
		if (!loaded || stat == null || user == null)
			tv.setText(getString(R.string.loading));
		else {
			if (user.isT)
				tv.setText(getString(R.string.clients_hour) + ": " + ((Integer)stat.users_hour).toString());
			else
				tv.setText(getString(R.string.translators) + ": " + ((Integer)stat.translators).toString());
		}
*/
        if (actionBar == null)
            return;
        if (user.isT)
            actionBar.setTitle(getString(R.string.clients_hour) + ": " + ((Integer)stat.users_hour).toString());
        else
            actionBar.setTitle(getString(R.string.translators) + ": " + ((Integer)stat.translators).toString());
	}
	public void updateStatistic(Parser.PacketStatistic _stat, User user) {
		stat = _stat;
		updateView(user);
	}
	public void updateLoaded(boolean _loaded) {
		loaded = _loaded;
		updateView(null);
	}
	MainActivity activity = null;
	public void init(MainActivity act) {
		activity = act;
	}
}

