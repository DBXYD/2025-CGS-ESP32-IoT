jQuery(document).ready(function($) {

	$(".headroom").headroom({
		"tolerance": 20,
		"offset": 50,
		"classes": {
			"initial": "animated",
			"pinned": "slideDown",
			"unpinned": "slideUp"
		}
	});

});

function setSizeVariables() {
	var largeur = window.innerWidth || document.documentElement.clientWidth || document.body.clientWidth;
	var hauteur = window.innerHeight || document.documentElement.clientHeight || document.body.clientHeight;

	document.documentElement.style.setProperty('--largeur-page', largeur + 'px');
	document.documentElement.style.setProperty('--hauteur-page', hauteur + 'px');
}
window.addEventListener("load", setSizeVariables);
window.addEventListener("resize", setSizeVariables);