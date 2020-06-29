##! Allows configuration of a pager email address to which notices can be sent.

@load ../main

module Notice;

export {
	redef enum Action += {
		## Indicates that the notice should be sent to the pager email
		## address configured in the :zeek:id:`Notice::mail_page_dest`
		## variable.
		ACTION_PAGE
	};
	
	## Email address to send notices with the :zeek:enum:`Notice::ACTION_PAGE`
	## action.
	option mail_page_dest = "";
}

hook notice(n: Notice::Info) &priority=-5
	{
	if ( ACTION_PAGE in n$actions )
		email_notice_to(n, mail_page_dest, F);
	}
