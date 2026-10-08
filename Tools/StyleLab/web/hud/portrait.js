// The HUD's portrait of the player character, as inline SVG so its parts can be moved and faded: the bust (glass, flat
// fills offset like a misregistered print, rim light, ink lines) and the four eye layers. It is the art of
// Art/Icons/HudPortrait.svg (same paths) with the mockup's extra eye layers, from Docs/HudMockup/NewHud.dc.html.
// Ids are unprefixed here; markup.js prefixes every id in the page's markup.
export const BUST = `<g id="bust">
<rect x="-10" y="-10" width="220" height="220" fill="url(#pGlass)"></rect>
<rect x="-10" y="-10" width="220" height="220" fill="url(#pScan)"></rect>
<g transform="translate(1.3 0.9)">
<path d="M-6 206 L-6 162 C14 150 38 141 62 137 L138 137 C162 141 186 150 206 162 L206 206 Z" fill="#33414d"></path>
<path d="M-6 162 C14 150 38 141 62 137 L138 137 C162 141 186 150 206 162 L206 168 C186 157 162 148 138 145 L62 145 C38 148 14 157 -6 168 Z" fill="#5b6e80"></path>
<path d="M84 206 L88 150 L100 166 L112 150 L116 206 Z" fill="#7f8a92"></path>
<path d="M48 206 L52 150 L58 118 L80 128 L88 150 L84 206 Z" fill="#46576a"></path>
<path d="M152 206 L148 150 L142 118 L120 128 L112 150 L116 206 Z" fill="#46576a"></path>
<path d="M58 118 L80 128 L77 136 L57 127 Z M142 118 L120 128 L123 136 L143 127 Z" fill="#6f8496"></path>
<path d="M70 74 L130 74 L133 96 C133 104 131 109 128 113 L72 113 C69 109 67 104 67 96 Z" fill="#9fb0bd"></path>
<path d="M67 74 L133 74 L133.5 93 C123 97.5 111.5 98.5 100 98 C88.5 98.5 77 97.5 66.5 93 Z" fill="#26343f"></path>
<path d="M97.8 98 L102.2 98 L103.4 105 L96.6 105 Z" fill="#c8d4dc"></path>
<path d="M66 76 C63 88 61 102 64 114 C66 108 68 104 70 102 C69 96 70 86 72 78 Z" fill="#141c23"></path>
<path d="M134 76 C137 88 139 102 136 114 C134 108 132 104 130 102 C131 96 130 86 128 78 Z" fill="#141c23"></path>
<path d="M66 105 C77 101.5 89 98.5 100 98 C111 98.5 123 101.5 134 105 L135.5 118 C128 131 116 146 100 166 C84 146 72 131 64.5 118 Z" fill="#ff5b4a"></path>
<path d="M65.5 121 C73 133 85 148 100 166 C115 148 127 133 134.5 121 C123 128 112 131 100 131 C88 131 77 128 65.5 121 Z" fill="#b83a2d"></path>
<path d="M64 63 C62 50 64 38 70.5 30 C77 24 87 20.5 95 22.5 L100 27.5 L105 22.5 C113 20.5 123 24 129.5 30 C136 38 138 50 136 63 C124 65.6 112 66.6 100 66.6 C88 66.6 76 65.6 64 63 Z" fill="#4a5a6a"></path>
<path d="M64 63 C62 50 64 38 70.5 30 C74 27 78.5 24.5 83 23.5 C78.5 34 77 48 78 64.9 C73 64.5 68.5 63.8 64 63 Z" fill="#2c3844"></path>
<path d="M86 26 C92 23.5 96 24 100 27.5 C104 24 108 23.5 114 26 C110 26.5 106 28.5 100 33 C94 28.5 90 26.5 86 26 Z" fill="#6d8093"></path>
<path d="M64 55.5 C76 58.5 88 59.8 100 59.8 C112 59.8 124 58.5 136 55.5 L136.3 63 C124 65.6 112 66.6 100 66.6 C88 66.6 76 65.6 63.7 63 Z" fill="#141c23"></path>
<path d="M121 56.6 L125.8 61.4 L121 66.2 L116.2 61.4 Z" fill="#ff9f1c"></path>
<path d="M20 70 C36 60.5 66 56.5 100 58.5 C134 56.5 164 60.5 180 70 C174 77.5 160 81.5 141 81.5 C126 81.5 112 83.5 100 85.5 C88 83.5 74 81.5 59 81.5 C40 81.5 26 77.5 20 70 Z" fill="#56687a"></path>
<path d="M20 70 C26 77.5 40 81.5 59 81.5 C74 81.5 88 83.5 100 85.5 C112 83.5 126 81.5 141 81.5 C160 81.5 174 77.5 180 70 C171 74.5 157 77 141 77 C126 77 112 79 100 80.5 C88 79 74 77 59 77 C43 77 29 74.5 20 70 Z" fill="#1d2730"></path>
</g>
<path d="M21 69.5 C37 60 66 56 100 58 M64 62 C62 49 64 37.5 70.5 29.5 C77 23.5 87 20 95 22" fill="none" stroke="#7fd8ff" stroke-width="1.4" opacity="0.8" stroke-linecap="round"></path>
<path d="M-6 161 C14 149 38 140 62 136" fill="none" stroke="#7fd8ff" stroke-width="1.4" opacity="0.6" stroke-linecap="round"></path>
<g fill="none" stroke="#0a1218" stroke-linejoin="round" stroke-linecap="round">
<path d="M-6 162 C14 150 38 141 62 137 L138 137 C162 141 186 150 206 162" stroke-width="3"></path>
<path d="M48 206 L52 150 L58 118 L80 128 L88 150 L84 206 M152 206 L148 150 L142 118 L120 128 L112 150 L116 206" stroke-width="2.6"></path>
<path d="M88 150 L100 166 L112 150" stroke-width="2"></path>
<path d="M70 74 L67 96 C67 104 69 109 72 113 M130 74 L133 96 C133 104 131 109 128 113" stroke-width="2.4"></path>
<path d="M66 76 C63 88 61 102 64 114 C66 108 68 104 70 102 M134 76 C137 88 139 102 136 114 C134 108 132 104 130 102" stroke-width="2"></path>
<path d="M66 105 C77 101.5 89 98.5 100 98 C111 98.5 123 101.5 134 105 L135.5 118 C128 131 116 146 100 166 C84 146 72 131 64.5 118 Z" stroke-width="2.6"></path>
<path d="M100 99 C98.5 108 98 118 99 128 M86 102 C84 110 83 116 84.5 124 M115 102 C117 110 118 116 116.5 124" stroke-width="1.5"></path>
<path d="M64 63 C62 50 64 38 70.5 30 C77 24 87 20.5 95 22.5 L100 27.5 L105 22.5 C113 20.5 123 24 129.5 30 C136 38 138 50 136 63" stroke-width="3"></path>
<path d="M100 33 L100 46 M86 27 C90 34 90.5 40 89.5 47 M114 27 C110 34 109.5 40 110.5 47" stroke-width="1.4"></path>
<path d="M121 56.6 L125.8 61.4 L121 66.2 L116.2 61.4 Z" stroke-width="1.2"></path>
<path d="M20 70 C36 60.5 66 56.5 100 58.5 C134 56.5 164 60.5 180 70 C174 77.5 160 81.5 141 81.5 C126 81.5 112 83.5 100 85.5 C88 83.5 74 81.5 59 81.5 C40 81.5 26 77.5 20 70 Z" stroke-width="3"></path>
</g>
</g>`;

export const EYES = `<g id="eyesOpen">
<ellipse cx="86" cy="90" rx="17" ry="9" fill="url(#pEye)"></ellipse>
<ellipse cx="114" cy="90" rx="17" ry="9" fill="url(#pEye)"></ellipse>
<path d="M77.5 90.4 C82 88 88.5 87.2 94 88.6 C89 91.4 83 92.2 77.5 90.4 Z" fill="#f2fdff"></path>
<path d="M122.5 90.4 C118 88 111.5 87.2 106 88.6 C111 91.4 117 92.2 122.5 90.4 Z" fill="#f2fdff"></path>
</g>
<g id="browsCalm"><path d="M74 86.6 C80 85 87 84.6 94 85.4 M126 86.6 C120 85 113 84.6 106 85.4" fill="none" stroke="#0a1218" stroke-width="2.2" stroke-linecap="round"></path></g>
<g id="eyesHurt">
<ellipse cx="86" cy="90" rx="12" ry="6" fill="url(#pEye)" opacity="0.7"></ellipse>
<ellipse cx="114" cy="90" rx="12" ry="6" fill="url(#pEye)" opacity="0.7"></ellipse>
<path d="M78.5 90.2 C83 89.2 88.5 89 93 89.8 C88.5 90.9 83 91.1 78.5 90.2 Z" fill="#f2fdff"></path>
<path d="M121.5 90.2 C117 89.2 111.5 89 107 89.8 C111.5 90.9 117 91.1 121.5 90.2 Z" fill="#f2fdff"></path>
<path d="M74 84.6 C81 85.4 88 86.6 94.5 88.4 M126 84.6 C119 85.4 112 86.6 105.5 88.4" fill="none" stroke="#0a1218" stroke-width="2.4" stroke-linecap="round"></path>
</g>
<g id="eyesFlare">
<ellipse cx="86" cy="90" rx="28" ry="16" fill="url(#pEye)"></ellipse>
<ellipse cx="114" cy="90" rx="28" ry="16" fill="url(#pEye)"></ellipse>
<path d="M76.5 90.4 C81.5 87.2 88.5 86.2 95 88.2 C89.5 91.8 83 92.8 76.5 90.4 Z" fill="#ffffff"></path>
<path d="M123.5 90.4 C118.5 87.2 111.5 86.2 105 88.2 C110.5 91.8 117 92.8 123.5 90.4 Z" fill="#ffffff"></path>
</g>`;
