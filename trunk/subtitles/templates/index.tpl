{* Smarty *}

<div class="search" align="center">

	<form action="{$smarty.server.PHP_SELF}" method="GET">
	
	<h1>Search</h1>
	
	<table>
	<tr>
		<th>Title</th>
		<td><input class="text blueborder" type="text" name="text" value="{$text|escape:"quote"}" /></td>
	</tr>
	<tr>
		<th>Disc(s)</th>
		<td><input class="blueborder" type="text" name="discs" {if !empty($discs)}value="{$discs|escape:"quote"}"{/if} size="1" /></td>
	</tr>
	<tr>
		<th>Language</th>
		<td>
		<select class="blueborder" name="isolang">
		<option value=""{if $isolang_sel == ""} selected="selected"{/if}>Please select a language...</option>
		{foreach from=$isolang key=code item=name}
		<option value="{$code}"{if $code == $isolang_sel} selected="selected"{/if}>{$name|truncate:40:"...":true|escape:"html"}</option>
		{/foreach}
		</select>
		</td>
	</tr>
	{* TODO: Format *}
	</table>
	
	<div style="padding-top: 10px;" align="center">
		<input type="submit" value="Search" />
	</div>

	</form>

</div>

<div class="results">

{if !empty($movies)}

	<ol>
	{foreach from=$movies item=m}
		{if $browser != 'Opera'}<li>{/if} {* grrrr *}
		{if !empty($m.imdb)}<a href="http://www.imdb.com/title/tt{$m.imdb|string_format:"%07d"}/" class="imdb" target="_blank">[IMDb]</a>{/if}
		{if $browser == 'Opera'}<li>{/if}
	
		{include file="title.tpl" titles=$m.titles}<br>
		Last updated: <strong>{$m.updated|date_format:"%Y %b %e"}</strong><br>
		
		<br>
	
		<table cellpadding="0" cellspacing="0">
		<tr>
			<th width="40%">File</th>
			<th>Disc</th>
			<th>Date</th>
			<th>Format</th>
			<th>Language</th>
			<th>Uploader</th>
		</tr>
		{foreach from=$m.subs item=s}
		<tr>
			<td>
				{if !empty($s.files)}<span class="dlme">&rarr;{/if}
				<a href="dl.php?id={$s.id}&ticket={$ticket}">{$s.name|escape:"html"}</a>
				{if !empty($s.files)}&larr;</span>{/if}
			</td>
			<td>{$s.disc_no}/{$s.discs}</td>
			<td><nobr>{$s.date|date_format:"%Y %b %e"}</nobr></td>
			<td>{$s.format|escape:"html"}</td>
			<td>{$s.language|escape:"html"}</td>
			<td>
				{if empty($s.email)}
				{$s.nick|escape:"html"}
				{else}
				{mailto address=$s.email text=$s.nick encode="hex"}
				{/if}
			</td>
		</tr>
		{if !empty($s.notes)}
		<tr>
		<td colspan="6" class="notes"><strong>Notes:</strong> {$s.notes|escape:"html"}</td>
		</tr>
		{/if}		
		{/foreach}
		</table>
		<br>

		</li>
	{/foreach}
	</ol>
	
{elseif !empty($files)}

<p align="center">
	Your search did not match any subtitles, would you like to 
	{strip}<a href="ul.php?
	{foreach from=$files key=i item=file}
		{foreach from=$file key=param item=value}
			{$param}[{$i}]={$value|escape:"url"}&
		{/foreach}
	{/foreach}{/strip}">upload</a> instead?
</p>

{elseif !empty($message)}

<p align="center">{$message|escape:"html"}</p>

{/if}

</div>