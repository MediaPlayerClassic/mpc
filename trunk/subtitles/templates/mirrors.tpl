{* Smarty *}

{if !empty($mirrors)}

<ol>
{foreach from=$mirrors item=m}
<li>
	{strip}
	<a href="{$m.url|escape:"quote"}">
	{if !empty($m.name)}{$m.name|escape:"html"}{else}{$m.url|escape:"html"}{/if}
	</a>
	{/strip}
</li>
{/foreach}
</ol>

{else}

<p align="center">Sorry, no other mirrors are available right now.</p>

{/if}

<p align="center">Would you like to setup your own mirror? Drop me an {mailto address="gabest@gabest.org" text="email" encode="hex"}!</p>

		