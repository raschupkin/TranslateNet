package com.translatenet;

import android.app.Activity;
import android.net.Uri;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.RatingBar;
import android.widget.TextView;


/**
 * A simple {@link Fragment} subclass.
 * Activities that contain this fragment must implement the
 * {@link StatisticFragment.OnFragmentInteractionListener} interface
 * to handle interaction events.
 * Use the {@link StatisticFragment#newInstance} factory method to
 * create an instance of this fragment.
 */
public class StatisticFragment extends Fragment {

    public StatisticFragment() {
        // Required empty public constructor
    }

    void UpdateView() {
        if (getView() == null)
            return;
        if (Stat != null) {
            TextView UsersHourTV = (TextView) getView().findViewById(R.id.ST_UsersHourTextView);
            UsersHourTV.setText(String.valueOf(Stat.users_hour));
            TextView UsersDayTV = (TextView) getView().findViewById(R.id.ST_UsersDayTextView);
            UsersDayTV.setText(String.valueOf(Stat.users_day));
            TextView CallsHouTVr = (TextView) getView().findViewById(R.id.ST_CallsHourTextView);
            CallsHouTVr.setText(String.valueOf(Stat.calls_hour));
            TextView CallsDayTV = (TextView) getView().findViewById(R.id.ST_CallsDayTextView);
            CallsDayTV.setText(String.valueOf(Stat.calls_day));
            TextView TranslatorsTV = (TextView) getView().findViewById(R.id.ST_TranslatorsTextView);
            TranslatorsTV.setText(String.valueOf(Stat.translators));
        }
        User user = activity.getUser();
        if (user != null) {
            RatingBar ratingBar = (RatingBar) getView().findViewById(R.id.ST_RatingBar);
            ratingBar.setRating(user.rating);
            TextView RatingNumTV = (TextView) getView().findViewById(R.id.ST_RatingNum);
            RatingNumTV.setText("(" + String.valueOf(user.rating_num) + ")");
            TextView BalanceTV = (TextView) getView().findViewById(R.id.ST_BalanceTextView);
            BalanceTV.setText(activity.FormatPrice(user.balance));
        }
        if (TStat != null) {
            TextView RatingTopTV = (TextView) getView().findViewById(R.id.ST_RatingTopTextView);
            int rating_top = TStat.higher_rating_num*100/TStat.all_rating_num;
            RatingTopTV.setText(String.valueOf(rating_top) + "%");
            TextView MoneySumTV = (TextView) getView().findViewById(R.id.ST_MoneySumTextView);
            MoneySumTV.setText(activity.FormatPrice(TStat.money_sum));
            TextView CallNumTV = (TextView) getView().findViewById(R.id.ST_CallNumTextView);
            CallNumTV.setText(String.valueOf(TStat.call_num));
            TextView CallTimeSumTV = (TextView) getView().findViewById(R.id.ST_CallTimeSumTextView);
            CallTimeSumTV.setText(activity.FormatTime(TStat.call_time_sum));
        }
    }

    Parser.PacketStatistic Stat = null;
    public void setStat(Parser.PacketStatistic _Stat) {
        Stat = _Stat;
        if (getView() != null)
            if (Stat != null)
                UpdateView();
    }

    Parser.PacketTranslatorStatistic TStat = null;
    public void setTStat(Parser.PacketTranslatorStatistic _TStat) {
        TStat = _TStat;
        if (getView() != null)
            if (TStat != null)
                UpdateView();
    }

    public void FragmentOpened() {
        activity.sendPacket_GetTranslatorStatistic();
    }

    @Override
    public void onStart() {
        super.onStart();
        if (Stat != null)
            UpdateView();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        // Inflate the layout for this fragment
        return inflater.inflate(R.layout.fragment_statistic, container, false);
    }

    MainActivity activity = null;
    public void init(MainActivity act) {
        activity = act;
    }

}
