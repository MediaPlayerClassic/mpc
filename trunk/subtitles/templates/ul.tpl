{* Smarty *}

<div class="subul">

{if empty($file)}
<div class="warning">
	Warning: You are trying to upload subtitles without binding them to their appropriate movie files,
	please use <a href="http://sf.net/projects/guliverkli/" target="_blank">Media Player Classic</a> (v6483+)
	to launch this page!
</div>
{/if}

<form action="{$smarty.server.PHP_SELF}" method="POST" enctype="multipart/form-data">

	<h1>Movie information</h1>
	
	<table>
	
	<tr class="optional{if isset($err.imdb)} invalid{/if}">
		<th><a href="http://imdb.com/" target="_blank">IMDb</a> (given this url, Title #1 can be left blank)</th>
		<td>
		{if !empty($titles_imdb)}
			<input class="hidden" type="text" name="imdb" value="{$imdb|escape:"quotes"}" />
			<a href="{$imdb|escape:"quotes"}" target="_blank">{$imdb|escape:"html"}</a>
			[<a href="{$smarty.server.PHP_SELF}?clearimdb={$imdb_id}">edit</a>]<br>
			{include file="title.tpl" titles=$titles_imdb}<br>
		{else}
			<input class="text" type="text" name="imdb" value="{$imdb|escape:"quotes"}" />
		{/if}
		</td>
	</tr>
	
	{foreach from=$title key=i item=t}
	<tr class="{if $i == 0 && empty($titles_imdb)}required{else}optional{/if}{if isset($err.title[$i])} invalid{/if}">
		<th>Title #{$i+1}</th>
		<td><input class="text" type="text" name="title[{$i}]" value="{$t|escape:"quotes"}" /></td>
	</tr>
	{/foreach}
	
	</table>

	{if empty($titles_imdb)}
	<div style="padding-top: 10px;" align="center">
		<input type="submit" name="update" value="Update" />
	</div>
	{/if}
	
	<h1>Uploader</h1>

	<table>

	<tr class="required{if isset($err.nick)} invalid{/if}">
		<th>Nick name</th>
		<td><input class="text" type="text" name="nick" value="{$nick|escape:"quotes"}" /></td>
	</tr>

	<tr class="optional{if isset($err.email)} invalid{/if}">
		<th>Email address</th>
		<td><input class="text" type="text" name="email" value="{$email|escape:"quotes"}" /></td>
	</tr>

	</table>

	{foreach from=$subs item=n}
	<h1><a href="javascript:void(0)" onclick="flip('sub{$n}')">Subtitle #{$n+1}</a></h1>
	<div class="{if $n == 0 || $isolang_sel[$n] != "" || $format_sel[$n] != "" || !empty($discs[$n]) || !empty($disc_no[$n]) || !empty($file_sel[$n]) || $notes[$n] != ""}shown{else}hidden{/if}" id="sub{$n}">
	{include file="ul.sub.tpl"}
	</div>
	{/foreach}

	<div style="padding-top: 10px;" align="center">
		<input type="submit" name="submit" value="Submit" />
	</div>
</form>

</div>