package com.translatenet;


import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.RatingBar;
import android.widget.RelativeLayout;
import android.widget.TextView;

import java.util.ArrayList;


/**
 * A simple {@link Fragment} subclass.
 */
public class HistoryFragment extends Fragment {
    User user;
    public void setUser(User _user)	{
        if (_user == null)
            return;
        try {
            user = (User)_user.clone();
        } catch (Exception ex) {
        }
    }
    ArrayList<Call> callList;
    ArrayList<User> markList;
    class CallInfo {
        Call call;
        RelativeLayout callL;
        TextView TranslateLabelTV;
        TextView TranslateTV;
        TextView StartLabelTV;
        TextView StartTV;
        TextView LengthLabelTV;
        TextView LengthTV;
        TextView CostLabelTV;
        TextView CostTV;
    }
    class MarkInfo {
        User translator;
        RelativeLayout markL;
        TextView NameLabelTV;
        TextView NameTV;
        TextView RatingLabelTV;
        RatingBar RatingBar;
        TextView RatingNumTV;
        Button RateB;
        LinearLayout callInfoListL;
        ArrayList<CallInfo> callInfoList = new ArrayList<CallInfo>();
    }

    protected void onButtonRate(MarkInfo ti) {
        Parser parser = new Parser();
        Parser.PacketMarkRequest p = parser.new PacketMarkRequest();
        p.translator = ti.translator.id;
        p.name = ti.translator.name;
        p.time = null;
        for (int i=0; i<callList.size(); i++) {
            Call c = callList.get(i);
            if (c.translator.id == p.translator) {
                if (p.time == null || c.start.after(p.time))
                    p.time = c.start;
            }
        }
        if (p.time != null)
            activity.OpenMarkDialog(p);
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
    protected MarkInfo addMarkItem(LinearLayout ll, User t) {
        MarkInfo ti = new MarkInfo();
        ti.translator = t;
        ti.markL = new RelativeLayout(activity.getApplicationContext());
        RelativeLayout.LayoutParams rlp = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.WRAP_CONTENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
        ti.markL.setLayoutParams(rlp);
        ll.addView(ti.markL);
        ti.markL.setTag(ti);

        ti.NameLabelTV = new TextView(getView().getContext());
        ti.NameLabelTV.setId(generateId());
        RelativeLayout.LayoutParams lp = new RelativeLayout.LayoutParams(FrameLayout.LayoutParams.WRAP_CONTENT, FrameLayout.LayoutParams.WRAP_CONTENT);
        ti.NameLabelTV.setLayoutParams(lp);
        ti.NameLabelTV.setTextSize(TypedValue.COMPLEX_UNIT_SP, FontSize);
        ti.NameLabelTV.setPadding(padding_hor, padding_vert, 0, 0);
        ti.NameLabelTV.setText(getString(R.string.name));
        ti.markL.addView(ti.NameLabelTV);

        ti.NameTV = new TextView(getView().getContext());
        ti.NameTV.setId(generateId());
        lp = new RelativeLayout.LayoutParams(FrameLayout.LayoutParams.WRAP_CONTENT, FrameLayout.LayoutParams.WRAP_CONTENT);
        lp.addRule(RelativeLayout.RIGHT_OF, ti.NameLabelTV.getId());
        ti.NameTV.setLayoutParams(lp);
        ti.NameTV.setTextSize(TypedValue.COMPLEX_UNIT_SP, FontSize);
        ti.NameTV.setPadding(padding_hor, padding_vert, 0, 0);
        ti.NameTV.setText(t.name);
        ti.markL.addView(ti.NameTV);

        ti.RatingLabelTV = new TextView(getView().getContext());
        ti.RatingLabelTV.setId(generateId());
        lp = new RelativeLayout.LayoutParams(FrameLayout.LayoutParams.WRAP_CONTENT, FrameLayout.LayoutParams.WRAP_CONTENT);
        lp.addRule(RelativeLayout.BELOW, ti.NameLabelTV.getId());
        ti.RatingLabelTV.setLayoutParams(lp);
        ti.RatingLabelTV.setTextSize(TypedValue.COMPLEX_UNIT_SP, FontSize);
        ti.RatingLabelTV.setPadding(padding_hor, padding_vert, 0, 0);
        ti.RatingLabelTV.setText(getString(R.string.last_rating));
        ti.markL.addView(ti.RatingLabelTV);

        RelativeLayout.LayoutParams ratinglp = new RelativeLayout.LayoutParams(FrameLayout.LayoutParams.WRAP_CONTENT, FrameLayout.LayoutParams.WRAP_CONTENT);
        ratinglp.addRule(RelativeLayout.BELOW, ti.NameLabelTV.getId());
        if (t.rating_num != 0) {
            ti.RatingBar = new RatingBar(getView().getContext(), null, android.R.attr.ratingBarStyleSmall);
            ti.RatingBar.setId(generateId());
            lp = new RelativeLayout.LayoutParams(FrameLayout.LayoutParams.WRAP_CONTENT, FrameLayout.LayoutParams.WRAP_CONTENT);
            lp.addRule(RelativeLayout.BELOW, ti.NameLabelTV.getId());
            lp.addRule(RelativeLayout.RIGHT_OF, ti.RatingLabelTV.getId());
            lp.setMargins(0, (int)(padding_vert*1.5), 0, 0);
            ti.RatingBar.setLayoutParams(lp);
            ti.RatingBar.setIsIndicator(true);
            if (t.rating_num > 0)
                ti.RatingBar.setRating(((float)t.rating)/20);
            ti.markL.addView(ti.RatingBar);
            ratinglp.addRule(RelativeLayout.RIGHT_OF, ti.RatingBar.getId());
        } else {
            ratinglp.addRule(RelativeLayout.RIGHT_OF, ti.RatingLabelTV.getId());
        }

        ti.RatingNumTV = new TextView(getView().getContext());
        ti.RatingNumTV.setId(generateId());
        ti.RatingNumTV.setLayoutParams(ratinglp);
        ti.RatingNumTV.setTextSize(TypedValue.COMPLEX_UNIT_SP, FontSize);
        ti.RatingNumTV.setPadding(padding_hor, padding_vert, 0, 0);
        ti.RatingNumTV.setText("(" + t.rating_num + ")");
        ti.markL.addView(ti.RatingNumTV);

        if (!user.isT) {
            ti.RateB = new Button(getView().getContext());
            ti.RateB.setId(generateId());
            ti.RateB.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    LinearLayout ll = (LinearLayout) getView().findViewById(R.id.H_CallListLayout);
                    if (ll == null)
                        return;
                    for (int i = 0; i < ll.getChildCount(); i++) {
                        MarkInfo ti = (MarkInfo) ll.getChildAt(i).getTag();
                        if (ti == null)
                            continue;
                        if (ti.RateB == v) {
                            onButtonRate(ti);
                        }
                    }
                }
            });
            lp = new RelativeLayout.LayoutParams(FrameLayout.LayoutParams.WRAP_CONTENT, FrameLayout.LayoutParams.WRAP_CONTENT);
            lp.addRule(RelativeLayout.BELOW, ti.NameLabelTV.getId());
            lp.addRule(RelativeLayout.RIGHT_OF, ti.RatingNumTV.getId());
            ti.RateB.setLayoutParams(lp);
            ti.RateB.setEnabled(false);
            ti.RateB.setText(getString(R.string.rate));
            ti.markL.addView(ti.RateB);
        }

        ti.callInfoListL = new LinearLayout(activity.getApplicationContext());
        ti.callInfoListL.setOrientation(LinearLayout.VERTICAL);
        lp = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.WRAP_CONTENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
        lp.addRule(RelativeLayout.BELOW, ti.RatingLabelTV.getId());
        lp.setMargins(30, 0, 0, 0);
        ti.callInfoListL.setLayoutParams(lp);
        ti.markL.addView(ti.callInfoListL);
        return ti;
    }

    protected CallInfo addCallItem(MarkInfo ti, Call call) {
        if (ti == null || call == null)
            return null;
        CallInfo ci = new CallInfo();
        ci.call = call;
        ci.callL = new RelativeLayout(activity.getApplicationContext());
        RelativeLayout.LayoutParams rlp = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.WRAP_CONTENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
        ci.callL.setLayoutParams(rlp);
        ti.callInfoListL.addView(ci.callL);
        ci.callL.setTag(ci);

        ci.TranslateLabelTV = new TextView(getView().getContext());
        ci.TranslateLabelTV.setId(generateId());
        RelativeLayout.LayoutParams lp = new RelativeLayout.LayoutParams(FrameLayout.LayoutParams.WRAP_CONTENT, FrameLayout.LayoutParams.WRAP_CONTENT);
        ci.TranslateLabelTV.setLayoutParams(lp);
        ci.TranslateLabelTV.setTextSize(TypedValue.COMPLEX_UNIT_SP, FontSize);
        ci.TranslateLabelTV.setPadding(padding_hor, padding_vert, 0, 0);
        ci.TranslateLabelTV.setText(getString(R.string.translate));
        ci.callL.addView(ci.TranslateLabelTV);

        ci.TranslateTV = new TextView(getView().getContext());
        ci.TranslateTV.setId(generateId());
        lp = new RelativeLayout.LayoutParams(FrameLayout.LayoutParams.WRAP_CONTENT, FrameLayout.LayoutParams.WRAP_CONTENT);
        lp.addRule(RelativeLayout.RIGHT_OF, ci.TranslateLabelTV.getId());
        ci.TranslateTV.setLayoutParams(lp);
        ci.TranslateTV.setTextSize(TypedValue.COMPLEX_UNIT_SP, FontSize);
        ci.TranslateTV.setPadding(padding_hor, padding_vert, 0, 0);
        ci.TranslateTV.setText(Lang.CodeToLang(call.TranslateLang));
        ci.callL.addView(ci.TranslateTV);

        ci.StartLabelTV = new TextView(getView().getContext());
        ci.StartLabelTV.setId(generateId());
        lp = new RelativeLayout.LayoutParams(FrameLayout.LayoutParams.WRAP_CONTENT, FrameLayout.LayoutParams.WRAP_CONTENT);
        lp.addRule(RelativeLayout.BELOW, ci.TranslateLabelTV.getId());
        ci.StartLabelTV.setLayoutParams(lp);
        ci.StartLabelTV.setTextSize(TypedValue.COMPLEX_UNIT_SP, FontSize);
        ci.StartLabelTV.setPadding(padding_hor, 0, 0, 0);
        ci.StartLabelTV.setText(getString(R.string.call_start_noun));
        ci.callL.addView(ci.StartLabelTV);

        ci.StartTV = new TextView(getView().getContext());
        ci.StartTV.setId(generateId());
        lp = new RelativeLayout.LayoutParams(FrameLayout.LayoutParams.WRAP_CONTENT, FrameLayout.LayoutParams.WRAP_CONTENT);
        lp.addRule(RelativeLayout.BELOW, ci.TranslateLabelTV.getId());
        lp.addRule(RelativeLayout.RIGHT_OF, ci.StartLabelTV.getId());
        ci.StartTV.setLayoutParams(lp);
        ci.StartTV.setTextSize(TypedValue.COMPLEX_UNIT_SP, FontSize);
        ci.StartTV.setPadding(padding_hor, 0, 0, 0);
        ci.StartTV.setText(activity.FormatDate(call.start));
        ci.callL.addView(ci.StartTV);

        ci.LengthLabelTV = new TextView(getView().getContext());
        ci.LengthLabelTV.setId(generateId());
        lp = new RelativeLayout.LayoutParams(FrameLayout.LayoutParams.WRAP_CONTENT, FrameLayout.LayoutParams.WRAP_CONTENT);
        lp.addRule(RelativeLayout.BELOW, ci.StartLabelTV.getId());
        ci.LengthLabelTV.setLayoutParams(lp);
        ci.LengthLabelTV.setTextSize(TypedValue.COMPLEX_UNIT_SP, FontSize);
        ci.LengthLabelTV.setPadding(padding_hor, 0, 0, 0);
        ci.LengthLabelTV.setText(getString(R.string.length));
        ci.callL.addView(ci.LengthLabelTV);

        ci.LengthTV = new TextView(getView().getContext());
        ci.LengthTV .setId(generateId());
        lp = new RelativeLayout.LayoutParams(FrameLayout.LayoutParams.WRAP_CONTENT, FrameLayout.LayoutParams.WRAP_CONTENT);
        lp.addRule(RelativeLayout.BELOW, ci.StartLabelTV.getId());
        lp.addRule(RelativeLayout.RIGHT_OF, ci.LengthLabelTV.getId());
        ci.LengthTV .setLayoutParams(lp);
        ci.LengthTV .setTextSize(TypedValue.COMPLEX_UNIT_SP, FontSize);
        ci.LengthTV .setPadding(padding_hor, 0, 0, 0);
        ci.LengthTV .setText(activity.FormatTime(call.length));
        ci.callL.addView(ci.LengthTV);

        ci.CostLabelTV = new TextView(getView().getContext());
        ci.CostLabelTV.setId(generateId());
        lp = new RelativeLayout.LayoutParams(FrameLayout.LayoutParams.WRAP_CONTENT, FrameLayout.LayoutParams.WRAP_CONTENT);
        lp.addRule(RelativeLayout.BELOW, ci.LengthLabelTV.getId());
        ci.CostLabelTV.setLayoutParams(lp);
        ci.CostLabelTV.setTextSize(TypedValue.COMPLEX_UNIT_SP, FontSize);
        ci.CostLabelTV.setPadding(padding_hor, 0, 0, 0);
        ci.CostLabelTV.setText(getString(R.string.cost));
        ci.callL.addView(ci.CostLabelTV);

        ci.CostTV = new TextView(getView().getContext());
        ci.CostTV.setId(generateId());
        lp = new RelativeLayout.LayoutParams(FrameLayout.LayoutParams.WRAP_CONTENT, FrameLayout.LayoutParams.WRAP_CONTENT);
        lp.addRule(RelativeLayout.BELOW, ci.LengthLabelTV.getId());
        lp.addRule(RelativeLayout.RIGHT_OF, ci.CostLabelTV.getId());
        ci.CostTV.setLayoutParams(lp);
        ci.CostTV.setTextSize(TypedValue.COMPLEX_UNIT_SP, FontSize);
        ci.CostTV.setPadding(padding_hor, 0, 0, 0);
        ci.CostTV.setText(activity.FormatPrice(call.cost));
        ci.callL.addView(ci.CostTV);

        if (!user.isT) {
            if (call.length > MainActivity.getOptions().CallMinTimeRating)
                if (ti.RateB != null)
                    ti.RateB.setEnabled(true);
        }
        return ci;
    }

    protected MarkInfo addTranslatorItem(int translator) {
        User t = findTranslator(translator);
        if (t == null)
            return null;

        LinearLayout ll = (LinearLayout)getView().findViewById(R.id.H_CallListLayout);
        if (ll == null)
            return null;

        MarkInfo ti = addMarkItem(ll, t);
        if (ti == null)
            return null;

        for (int i=0; i<callList.size(); i++) {
            Call call = callList.get(i);
            if (call.translator.id != translator && !user.isT)
                continue;

            CallInfo ci = addCallItem(ti, call);
            ti.callInfoList.add(ci);

            call.displayed = true;
        }
        return ti;
    }

    protected User findTranslator(int translator) {
        for (int i=0; i< markList.size(); i++) {
            User t =  markList.get(i);
            if (t.id == translator)
                return t;
        }
        for (int i=0; i<callList.size(); i++) {
            User t = callList.get(i).translator;
            if (t.id == translator)
                return t;
        }
        return null;
    }

    protected void SortCallList(ArrayList<Call> list) {
        for (int i=0; i<list.size(); i++)
            for (int j=i+1; j<list.size(); j++) {
                if (list.get(i).start.before(list.get(j).start)) {
                    Call c = list.get(j);
                    list.set(j,list.get(i));
                    list.set(i, c);
                }
            }
    }
    protected void UpdateCallList() {
        SortCallList(callList);
        if (user.isT) {
             markList.add(user);
            addTranslatorItem(user.id);
        } else
            for (int i=0; i<callList.size(); i++) {
                Call c = callList.get(i);
                if (c.displayed)
                    continue;
                if (user == null)
                    continue;
                addTranslatorItem(c.translator.id);
            }
    }
    protected void ClearCallList() {
        if (getView() == null)
            return;
        LinearLayout ll = (LinearLayout)getView().findViewById(R.id.H_CallListLayout);
        if (ll == null)
            return;
        ll.removeAllViews();
    }

    public void onPacketMarkHistory(ArrayList<User> _markList) {
         markList = _markList;
        if ( markList != null && callList != null) {
            activity.showProgress(false);
            UpdateCallList();
        }
    }

    public void onPacketCallHistory(ArrayList<Call> _callList) {
        callList = _callList;
        if ( markList != null && callList != null) {
            UpdateCallList();
            activity.showProgress(false);
        }
    }

    protected void ReloadMarkLists() {
        callList = null;
        markList = null;
        activity.showProgress(true);

        activity.sendPacket_GetCallHistory();
        activity.sendPacket_GetMarkHistory();
    }

    public void FragmentOpened() {
        if (getView() != null)
            ClearCallList();
        ReloadMarkLists();
    }

    @Override
    public void onStart() {
        super.onStart();
    }

    public HistoryFragment() {
        // Required empty public constructor
    }


    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        // Inflate the layout for this fragment
        View v = inflater.inflate(R.layout.fragment_history, container, false);
        Button DoneButton = (Button)v.findViewById(R.id.H_DoneButton);
        DoneButton.setOnClickListener(new View.OnClickListener()  {
            @Override
            public void onClick(View v) {
                if (activity == null)
                    return;
                activity.CloseHistory();
            }
        });
        return v;
    }

    MainActivity activity = null;
    public void init(MainActivity act) {
        activity = act;
    }

}
