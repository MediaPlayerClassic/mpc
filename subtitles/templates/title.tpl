{* Smarty *}

{strip}
{foreach from=$titles key=i item=t}
{if $i > 0}, aka {/if}<strong>{$t}</strong>
{/foreach}
{/strip}