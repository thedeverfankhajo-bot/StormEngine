(() => {
  "use strict";

  const translations = {
    en: {
      skip:"Skip to content", "brand-tag":"Independent game technology", "nav-label":"Primary navigation",
      "nav-architecture":"Architecture", "nav-roadmap":"Roadmap", "nav-docs":"Docs", "nav-source":"Source ↗", "move-up":"Move up", "move-left":"Move left", "move-down":"Move down", "move-right":"Move right",
      "hero-eyebrow":"EARLY STAGE · BUILT IN THE OPEN", "hero-title-a":"One engine.", "hero-title-b":"Every dimension.",
      "hero-lede":"A native, modular foundation for 1D, 2D and 3D worlds. Built with portability, clear module boundaries and tested behavior at the center.",
      "hero-cta":"Explore the architecture ↘", "repo-cta":"View repository ↗",
      "reality-note":"This is an early foundation—not yet a production-complete engine or an Unreal Engine 5 equivalent.",
      "visual-label":"A central runtime joining one-dimensional, two-dimensional and three-dimensional worlds",
      "dimension-signals":"Signals", "dimension-worlds":"Worlds", "dimension-space":"Space",
      "visual-caption":"ONE CODEBASE · CLEAR MODULE BOUNDARIES", "metric-native":"Native foundation",
      "metric-scenes":"Dimension-aware scenes", "metric-deps":"Third-party core runtime dependencies", "metric-license":"Open-source project license",
      "demo-kicker":"INTERACTIVE LAB / 001", "demo-title":"A small world you can move.",
      "demo-intro":"Move the marker with WASD, arrow keys, the buttons below, or tap the playfield. This is a browser illustration—not a build of the C++ runtime.",
      "demo-ready":"Sandbox ready", "demo-paused":"Motion paused", pause:"Pause motion", resume:"Resume motion", reset:"Reset position",
      "direction-label":"Move demo marker", live:"LIVE", "canvas-label":"Interactive two-dimensional demo. Use arrow keys or WASD to move the marker.",
      "player-entity":"Player entity", "world-origin":"World origin",
      "projection-kicker":"INTERACTIVE LAB / 002", "projection-title":"Shape a perspective camera.",
      "projection-intro":"Change the camera settings and inspect the resulting right-handed perspective matrix. Near and far depth mappings update live; the diagram is illustrative and the matrix values are authoritative.",
      "projection-controls-label":"Perspective projection controls", "projection-fov-label":"Vertical field of view", "projection-fov-help":"A larger angle shows more of the world but increases perspective distortion.",
      "projection-aspect-label":"Viewport aspect ratio", "projection-aspect-16-9":"16:9 · widescreen", "projection-aspect-16-10":"16:10",
      "projection-aspect-4-3":"4:3", "projection-aspect-1-1":"1:1 · square", "projection-near-label":"Near plane", "projection-far-label":"Far plane",
      "projection-depth-label":"Normalized-device depth range", "projection-depth-zero":"Zero to one · [0, 1]", "projection-depth-negative":"Negative one to one · [-1, 1]",
      "projection-depth-help":"Select the convention explicitly. StormEngine does not guess the graphics backend's clip-space depth range.",
      "projection-preview-label":"Frustum preview", "projection-diagram-label":"Illustrative perspective frustum diagram",
      "projection-canvas-fallback":"A perspective-frustum illustration. Matrix results remain available below.",
      "projection-near-result":"Near plane → NDC z = {value}", "projection-far-result":"Far plane → NDC z = {value}",
      "projection-matrix-title":"Projection matrix · row view", "projection-matrix-label":"Perspective projection matrix",
      "projection-footnote":"The matrix is displayed by rows for readability; StormEngine's Matrix4 stores values column-major. The demo does not call a GPU API.",
      "projection-ready":"Projection is valid. Near and far planes map to the selected depth range.",
      "projection-invalid-planes":"Invalid planes: near must be positive and far must be greater than near.",
      "projection-invalid-math":"This input combination exceeds the finite range supported by the engine's 32-bit float matrix.",
      "projection-camera":"CAMERA", "projection-near-name":"NEAR", "projection-far-name":"FAR", "projection-not-to-scale":"ILLUSTRATIVE · NOT TO SCALE",
      "arch-kicker":"DESIGNED TO GROW", "arch-title":"One runtime, focused modules.",
      "arch-intro":"Build hard foundations first. Keep platform, rendering, scene data and tools separated so each can evolve without destabilizing the core.",
      "state-building":"Building blocks", "state-early":"Early implementation", "state-reference":"Reference prototype", "state-planned":"Planned",
      "mod-core":"Core & Math", "mod-core-desc":"Versioning, status results, stable IDs, fixed-step timing, transforms, vectors, matrices and quaternion rotations.",
      "link-headers":"Explore headers ↗", "mod-scene":"Scene & Project",
      "mod-scene-desc":"Dimension-aware entities and transforms, versioned binary serialization, validated logical paths.",
      "link-scene":"Explore scene APIs ↗", "mod-render":"Rendering",
      "mod-render-desc":"Bounded CPU 2D canvas, orthographic camera, texture sampling and scene-renderer composition.",
      "link-render":"Explore render APIs ↗", "mod-platform":"Platform & Input",
      "mod-platform-desc":"Bounded input/event primitives and optional SDL3 desktop bootstrap; full lifecycle adapters remain ahead.",
      "link-build":"Build guide ↗", "mod-assets":"Assets & Import",
      "mod-assets-desc":"The texture registry is in-memory today. Import, metadata, caching, decoding and cooked packages need design.",
      "link-roadmap":"Asset roadmap ↗", "mod-future":"3D, Physics & Editor",
      "mod-future-desc":"Quaternion and explicit perspective projection math are implemented; production RHI, GPU rendering, physics, audio, scripting and editor workflows remain future milestones.",
      "link-future":"Future milestones ↗", "arch-footnote":"These labels describe the repository today—not feature parity or certification. Each roadmap milestone needs evidence in code, tests and documentation.",
      "quality-kicker":"ENGINEERING OVER CHECKLISTS", "quality-title":"Reliability is a feature.",
      "quality-intro":"Bounded resource use, failure-aware APIs and repeatable tests come before breadth.",
      "quality-bounds":"Bounded data", "quality-bounds-desc":"Scene records, file IO, CPU textures and event queues have explicit limits and validation.",
      "quality-portable":"Portable builds", "quality-portable-desc":"CMake and C++20 target Linux, macOS, Windows and Android arm64 native-core CI.",
      "quality-tests":"Tests & sanitizers", "quality-tests-desc":"CTest, package-consumer coverage and AddressSanitizer/UBSan help guard regressions.",
      "quality-supply":"Supply-chain hygiene", "quality-supply-desc":"SDL is checksum-pinned, workflow actions are pinned by commit, and the default core stays lean.",
      "roadmap-kicker":"INCREMENTAL MILESTONES", "roadmap-title-a":"Prove each layer.", "roadmap-title-b":"Then move forward.",
      "roadmap-intro":"The roadmap is an engineering contract, not a date promise. A milestone moves when acceptance criteria are shown in code and tests.",
      "full-roadmap":"Read the full roadmap ↗", "milestone-established":"Established",
      "milestone-primitives":"Primitives implemented", "milestone-prototype":"Prototype stage",
      mile0:"Repository foundation", "mile0-desc":"C++20 core, CMake package, baseline CI, tests and architecture guidance.",
      mile1:"Runtime kernel", "mile1-desc":"Timing, lifecycle, status/error model, stable IDs, logging and bounded diagnostics.",
      mile2:"Platform shell & 2D slice", "mile2-desc":"A testable CPU canvas and optional SDL desktop sample exist; production rendering is still ahead.",
      mile3:"Asset & scene pipeline", "mile3-desc":"Import validation, project-root-confined IO, metadata, cache and cooked assets.",
      mile4:"3D rendering & simulation", "mile4-desc":"Benchmark-led backend choice, meshes, materials, shaders, cameras and physics integration.",
      mile5:"Android application runtime", "mile5-desc":"NDK cross-build is not an APK. App lifecycle, packaging and real-device tests remain required.",
      "start-kicker":"BUILD IT LOCALLY", "start-title":"Start with the core.",
      "start-intro":"You need CMake 3.25+ and a C++20 compiler. The default configuration does not fetch third-party runtime code.",
      "start-guide":"Full build instructions ↗", "terminal-label":"TERMINAL / CORE BUILD", "copy-command":"Copy commands",
      "copy-success":"Copied", "copy-failed":"Copy failed",
      "code-footnote":"The optional SDL3 demo is a separate desktop target. It is not an Android APK or production renderer.",
      "docs-kicker":"READ THE SOURCE OF TRUTH", "docs-title":"Documentation, close to code.",
      "search-label":"Search documentation", "search-placeholder":"Search docs…",
      "search-summary":"Browse the engineering references or search by topic.", "search-count":"Showing {shown} of {total} documents",
      "docs-empty":"No document matches that search. Try a different keyword.",
      "doc-overview":"Project overview", "doc-overview-desc":"Current capabilities, quick start and project limitations.", "tag-overview":"Status · Setup · Scope",
      "doc-architecture":"Architecture", "doc-architecture-desc":"Module boundaries, invariants, ownership and current design decisions.", "tag-architecture":"Design · Modules · API",
      "doc-roadmap":"Roadmap", "doc-roadmap-desc":"Milestones and acceptance criteria for the next engine layers.", "tag-roadmap":"Milestones · Planned work",
      "doc-build":"Build & install", "doc-build-desc":"Desktop toolchains, CMake package, sanitizers and Android NDK.", "tag-build":"CMake · CI · Android",
      "doc-contrib":"Contributing", "doc-contrib-desc":"Coding standards, review practices, tests and contribution rules.", "tag-contrib":"Community · Quality",
      "doc-security":"Security policy", "doc-security-desc":"Vulnerability reporting, supported versions and secure engineering principles.", "tag-security":"Security · Reporting",
      "doc-development":"Development process", "doc-development-desc":"Start-of-task repository review and safe mainline workflow.", "tag-development":"Workflow · Review",
      "doc-thirdparty":"Third-party notices", "doc-thirdparty-desc":"Dependency provenance, versions, SHA-256 and separate license terms.", "tag-thirdparty":"Licenses · Provenance",
      "doc-license":"License", "doc-license-desc":"Apache-2.0 terms for StormEngine's original code and documentation.",
      "doc-projection":"Perspective projection", "doc-projection-desc":"Right-handed camera math, explicit clip-depth ranges and edge-case tests.", "tag-projection":"3D Math · Camera · Depth",
      "closing-kicker":"BUILD THE FOUNDATION WITH US", "closing-title":"Open code. Measured progress.",
      "closing-desc":"Review the source, follow the milestones, and help turn tested primitives into a dependable engine.",
      "closing-cta":"Go to GitHub ↗", "footer-tag":"Native game-engine foundations",
      "footer-disclaimer":"Independent project in active development. Current capabilities are described as implemented, not aspirational.",
      "footer-nav":"Legal and project links", "footer-license":"Licensed under Apache-2.0",
      "footer-privacy":"No analytics · No external fonts · No third-party scripts",
      "theme-light":"Switch to light theme", "theme-dark":"Switch to dark theme"
    },
    fa: {
      skip:"رفتن به محتوای اصلی", "brand-tag":"فناوری مستقل بازی", "nav-label":"پیمایش اصلی",
      "nav-architecture":"معماری", "nav-roadmap":"نقشهٔ راه", "nav-docs":"مستندات", "nav-source":"کد منبع ↗", "move-up":"حرکت به بالا", "move-left":"حرکت به چپ", "move-down":"حرکت به پایین", "move-right":"حرکت به راست",
      "hero-eyebrow":"مرحلهٔ اولیه · توسعهٔ شفاف", "hero-title-a":"یک موتور.", "hero-title-b":"تمام بُعدها.",
      "hero-lede":"پایه‌ای بومی و ماژولار برای جهان‌های یک‌بعدی، دوبعدی و سه‌بعدی؛ با تمرکز بر قابلیت حمل، مرزبندی روشن ماژول‌ها و رفتار آزموده‌شده.",
      "hero-cta":"مرور معماری ↘", "repo-cta":"مشاهدهٔ مخزن ↗",
      "reality-note":"این پروژه هنوز در مرحلهٔ پایه‌گذاری است؛ موتور کامل تجاری یا هم‌سطح Unreal Engine 5 نیست.",
      "visual-label":"یک هستهٔ زمان‌اجرا که جهان‌های یک‌بعدی، دوبعدی و سه‌بعدی را به هم پیوند می‌دهد",
      "dimension-signals":"سیگنال‌ها", "dimension-worlds":"جهان‌ها", "dimension-space":"فضا",
      "visual-caption":"یک کدبیس · مرزهای روشن ماژول‌ها", "metric-native":"پایهٔ بومی",
      "metric-scenes":"صحنه‌های چندبُعدی", "metric-deps":"وابستگی هستهٔ زمان‌اجرا به شخص ثالث", "metric-license":"مجوز متن‌باز پروژه",
      "demo-kicker":"آزمایشگاه تعاملی / ۰۰۱", "demo-title":"جهانی کوچک برای حرکت‌دادن.",
      "demo-intro":"نشانگر را با WASD، کلیدهای جهت‌نما، دکمه‌های پایین یا لمس میدان بازی حرکت بده. این تصویرسازی مرورگری است، نه نسخهٔ اجرایی موتور C++.",
      "demo-ready":"محیط آزمایش آماده است", "demo-paused":"حرکت متوقف شد", pause:"توقف حرکت", resume:"ادامهٔ حرکت", reset:"بازنشانی موقعیت",
      "direction-label":"حرکت‌دادن نشانگر آزمایشی", live:"زنده", "canvas-label":"دموی دوبعدی تعاملی؛ نشانگر را با کلیدهای جهت‌نما یا WASD حرکت بده.",
      "player-entity":"موجودیت بازیکن", "world-origin":"مبدأ جهان",
      "projection-kicker":"آزمایشگاه تعاملی / ۰۰۲", "projection-title":"دوربین پرسپکتیو را شکل بده.",
      "projection-intro":"پارامترهای دوربین را تغییر بده و ماتریس پرسپکتیو راست‌دست را ببین. نگاشت عمق صفحه‌های نزدیک و دور زنده تغییر می‌کند؛ شکل فقط نمایشی است و مقادیر ماتریس مرجع اصلی‌اند.",
      "projection-controls-label":"کنترل‌های تصویرسازی پرسپکتیو", "projection-fov-label":"میدان دید عمودی", "projection-fov-help":"زاویهٔ بزرگ‌تر بخش بیشتری از جهان را نشان می‌دهد، اما اعوجاج پرسپکتیو را بیشتر می‌کند.",
      "projection-aspect-label":"نسبت ابعاد نما", "projection-aspect-16-9":"۱۶:۹ · عریض", "projection-aspect-16-10":"۱۶:۱۰",
      "projection-aspect-4-3":"۴:۳", "projection-aspect-1-1":"۱:۱ · مربع", "projection-near-label":"صفحهٔ نزدیک", "projection-far-label":"صفحهٔ دور",
      "projection-depth-label":"بازهٔ عمق مختصات نرمال‌شده", "projection-depth-zero":"صفر تا یک · [۰، ۱]", "projection-depth-negative":"منفی یک تا یک · [۱-، ۱]",
      "projection-depth-help":"قرارداد را صریح انتخاب کن. StormEngine بازهٔ عمق فضای برش را برای بک‌اند گرافیکی حدس نمی‌زند.",
      "projection-preview-label":"پیش‌نمایش حجم دید", "projection-diagram-label":"نمودار نمایشی حجم دید پرسپکتیو",
      "projection-canvas-fallback":"تصویرسازی حجم دید پرسپکتیو؛ نتیجهٔ ماتریس هم در پایین موجود است.",
      "projection-near-result":"صفحهٔ نزدیک ← عمق نرمال‌شده z = {value}", "projection-far-result":"صفحهٔ دور ← عمق نرمال‌شده z = {value}",
      "projection-matrix-title":"ماتریس پرسپکتیو · نمایش سطری", "projection-matrix-label":"ماتریس تصویرسازی پرسپکتیو",
      "projection-footnote":"ماتریس برای خوانایی به‌شکل سطرها نمایش داده شده است؛ Matrix4 در StormEngine مقادیر را ستون‌به‌ستون ذخیره می‌کند. این دمو هیچ API گرافیکی را فراخوانی نمی‌کند.",
      "projection-ready":"پرسپکتیو معتبر است. صفحه‌های نزدیک و دور به بازهٔ عمق انتخاب‌شده نگاشت می‌شوند.",
      "projection-invalid-planes":"صفحه‌ها نامعتبرند: صفحهٔ نزدیک باید مثبت و صفحهٔ دور بزرگ‌تر از آن باشد.",
      "projection-invalid-math":"این ترکیب ورودی از محدودهٔ متناهی ماتریس ۳۲بیتی موتور فراتر می‌رود.",
      "projection-camera":"دوربین", "projection-near-name":"نزدیک", "projection-far-name":"دور", "projection-not-to-scale":"نمایشی · بدون مقیاس واقعی",
      "arch-kicker":"طراحی‌شده برای رشد", "arch-title":"یک زمان‌اجرا، ماژول‌های متمرکز.",
      "arch-intro":"ابتدا پایه‌های دشوار را بسازیم. پلتفرم، رندر، دادهٔ صحنه و ابزارها را جدا نگه می‌داریم تا مستقل از هسته تکامل یابند.",
      "state-building":"بلوک‌های پایه", "state-early":"پیاده‌سازی اولیه", "state-reference":"نمونهٔ مرجع", "state-planned":"برنامه‌ریزی‌شده",
      "mod-core":"هسته و ریاضیات", "mod-core-desc":"نسخه‌بندی، وضعیت و خطا، شناسه‌های پایدار، زمان‌بندی گام ثابت، بردارها، ماتریس‌ها و چرخش‌های کواترنیونی.",
      "link-headers":"مشاهدهٔ سربرگ‌ها ↗", "mod-scene":"صحنه و پروژه",
      "mod-scene-desc":"موجودیت و تبدیل چندبُعدی، سریال‌سازی دودویی نسخه‌دار و مسیرهای منطقی اعتبارسنجی‌شده.",
      "link-scene":"مشاهدهٔ API صحنه ↗", "mod-render":"رندرینگ",
      "mod-render-desc":"بوم دوبعدی محدود مبتنی بر CPU، دوربین ارتوگرافیک، نمونه‌برداری بافت و ترکیب رندر صحنه.",
      "link-render":"مشاهدهٔ API رندر ↗", "mod-platform":"پلتفرم و ورودی",
      "mod-platform-desc":"ساختارهای ورودی و رویداد محدود و نمونهٔ دسکتاپ اختیاری SDL3؛ آداپترهای کامل هنوز در راه‌اند.",
      "link-build":"راهنمای ساخت ↗", "mod-assets":"دارایی و واردسازی",
      "mod-assets-desc":"ثبت‌کنندهٔ بافت فعلاً درون‌حافظه‌ای است. واردسازی، فراداده، کش، رمزگشایی و بسته‌بندی نیاز به طراحی دارند.",
      "link-roadmap":"نقشهٔ راه دارایی‌ها ↗", "mod-future":"سه‌بعدی، فیزیک و ویرایشگر",
      "mod-future-desc":"چرخش کواترنیونی و تصویرسازی پرسپکتیو صریح پیاده‌سازی شده‌اند؛ RHI تولیدی، رندر GPU، فیزیک، صدا، اسکریپت و ویرایشگر هنوز در راه‌اند.",
      "link-future":"مراحل آینده ↗", "arch-footnote":"این برچسب‌ها وضعیت فعلی مخزن را نشان می‌دهند، نه برابری قابلیت یا گواهی. هر مرحله باید با کد، آزمون و مستندات اثبات شود.",
      "quality-kicker":"مهندسی فراتر از چک‌لیست", "quality-title":"قابلیت اعتماد یک ویژگی است.",
      "quality-intro":"استفادهٔ محدود از منابع، APIهای آگاه از خطا و آزمون‌های بازتولیدپذیر بر گستردگی قابلیت‌ها مقدم‌اند.",
      "quality-bounds":"دادهٔ محدودشده", "quality-bounds-desc":"رکوردهای صحنه، فایل، بافت CPU و صف رویدادها محدودیت و اعتبارسنجی صریح دارند.",
      "quality-portable":"ساخت قابل‌حمل", "quality-portable-desc":"CMake و C++20، لینوکس، macOS، ویندوز و ساخت هستهٔ بومی Android arm64 در CI را هدف می‌گیرند.",
      "quality-tests":"آزمون و Sanitizer", "quality-tests-desc":"CTest، آزمون مصرف بسته و AddressSanitizer/UBSan به جلوگیری از پسرفت کمک می‌کنند.",
      "quality-supply":"زنجیرهٔ تأمین سالم", "quality-supply-desc":"SDL با checksum ثابت دریافت می‌شود، Actionها به commit مشخص بسته‌اند و هستهٔ پیش‌فرض سبک می‌ماند.",
      "roadmap-kicker":"مراحل افزایشی", "roadmap-title-a":"هر لایه را ثابت کن.", "roadmap-title-b":"بعد جلو برو.",
      "roadmap-intro":"نقشهٔ راه یک قرارداد مهندسی است، نه وعدهٔ تاریخ. هر مرحله با اثبات معیارهای پذیرش در کد و آزمون پیش می‌رود.",
      "full-roadmap":"خواندن نقشهٔ راه کامل ↗", "milestone-established":"پایه‌گذاری‌شده",
      "milestone-primitives":"اجزای پایه پیاده‌سازی شده", "milestone-prototype":"مرحلهٔ نمونه‌سازی",
      mile0:"پایهٔ مخزن", "mile0-desc":"هستهٔ C++20، بستهٔ CMake، CI پایه، آزمون و راهنمای معماری.",
      mile1:"هستهٔ زمان‌اجرا", "mile1-desc":"زمان‌بندی، چرخهٔ عمر، وضعیت و خطا، شناسهٔ پایدار، گزارش‌گیری و عیب‌یابی محدود.",
      mile2:"پوستهٔ پلتفرم و برش دوبعدی", "mile2-desc":"بوم CPU آزمون‌پذیر و نمونهٔ دسکتاپ SDL وجود دارند؛ رندر تولیدی همچنان در راه است.",
      mile3:"خط لولهٔ دارایی و صحنه", "mile3-desc":"اعتبارسنجی واردسازی، فایل‌خوانی محدود به ریشهٔ پروژه، فراداده، کش و دارایی‌های بسته‌بندی‌شده.",
      mile4:"رندر سه‌بعدی و شبیه‌سازی", "mile4-desc":"انتخاب بک‌اند با بنچمارک، مش، متریال، شیدر، دوربین و یکپارچه‌سازی فیزیک.",
      mile5:"زمان‌اجرای اپلیکیشن اندروید", "mile5-desc":"ساخت NDK به‌تنهایی APK نیست؛ چرخهٔ عمر برنامه، بسته‌بندی و آزمون دستگاه واقعی لازم‌اند.",
      "start-kicker":"ساخت روی دستگاه خودت", "start-title":"از هسته شروع کن.",
      "start-intro":"به CMake نسخهٔ ۳٫۲۵ و کامپایلر C++20 نیاز است. ساخت پیش‌فرض کد زمان‌اجرای شخص ثالث دریافت نمی‌کند.",
      "start-guide":"راهنمای کامل ساخت ↗", "terminal-label":"ترمینال / ساخت هسته", "copy-command":"کپی دستورها",
      "copy-success":"کپی شد", "copy-failed":"کپی ناموفق بود",
      "code-footnote":"دموی اختیاری SDL3 هدف جداگانهٔ دسکتاپ است، نه APK اندروید یا رندر تولیدی.",
      "docs-kicker":"مرجع اصلی کد را بخوان", "docs-title":"مستندات، کنار کد.",
      "search-label":"جست‌وجو در مستندات", "search-placeholder":"جست‌وجوی مستندات…",
      "search-summary":"مراجع مهندسی را مرور کن یا براساس موضوع جست‌وجو کن.", "search-count":"نمایش {shown} از {total} سند",
      "docs-empty":"سندی با این عبارت پیدا نشد؛ واژهٔ دیگری را امتحان کن.",
      "doc-overview":"نمای کلی پروژه", "doc-overview-desc":"قابلیت‌های کنونی، شروع سریع و محدودیت‌های پروژه.", "tag-overview":"وضعیت · راه‌اندازی · محدوده",
      "doc-architecture":"معماری", "doc-architecture-desc":"مرز ماژول‌ها، ناورداها، مالکیت و تصمیم‌های طراحی.", "tag-architecture":"طراحی · ماژول · API",
      "doc-roadmap":"نقشهٔ راه", "doc-roadmap-desc":"مراحل و معیارهای پذیرش برای لایه‌های بعدی موتور.", "tag-roadmap":"مراحل · کار برنامه‌ریزی‌شده",
      "doc-build":"ساخت و نصب", "doc-build-desc":"زنجیره‌ابزار دسکتاپ، بستهٔ CMake، Sanitizer و Android NDK.", "tag-build":"CMake · CI · اندروید",
      "doc-contrib":"مشارکت", "doc-contrib-desc":"استاندارد کدنویسی، بازبینی، آزمون و قوانین مشارکت.", "tag-contrib":"جامعه · کیفیت",
      "doc-security":"سیاست امنیت", "doc-security-desc":"گزارش آسیب‌پذیری، نسخه‌های پشتیبانی و اصول مهندسی امن.", "tag-security":"امنیت · گزارش",
      "doc-development":"فرایند توسعه", "doc-development-desc":"بازبینی مخزن در آغاز کار و گردش‌کار امن شاخهٔ اصلی.", "tag-development":"گردش‌کار · بازبینی",
      "doc-thirdparty":"اعلان‌های شخص ثالث", "doc-thirdparty-desc":"منشأ وابستگی، نسخه، SHA-256 و شرایط مجوز جداگانه.", "tag-thirdparty":"مجوز · منشأ",
      "doc-license":"مجوز", "doc-license-desc":"شرایط Apache-2.0 برای کد و مستندات اصلی StormEngine.",
      "doc-projection":"تصویرسازی پرسپکتیو", "doc-projection-desc":"ریاضیات دوربین راست‌دست، محدودهٔ عمق صریح و آزمون حالت‌های مرزی.", "tag-projection":"ریاضیات سه‌بعدی · دوربین · عمق",
      "closing-kicker":"با هم پایه را بسازیم", "closing-title":"کد باز. پیشرفت قابل‌اندازه‌گیری.",
      "closing-desc":"کد را بازبینی کن، مراحل را دنبال کن و به تبدیل اجزای آزموده‌شده به موتوری قابل‌اعتماد کمک کن.",
      "closing-cta":"رفتن به گیت‌هاب ↗", "footer-tag":"پایه‌های موتور بازی بومی",
      "footer-disclaimer":"پروژه‌ای مستقل در حال توسعه؛ قابلیت‌های کنونی بر اساس پیاده‌سازی، نه آرزوها، توصیف شده‌اند.",
      "footer-nav":"پیوندهای حقوقی و پروژه", "footer-license":"دارای مجوز Apache-2.0",
      "footer-privacy":"بدون آمارگیر · بدون فونت خارجی · بدون اسکریپت شخص ثالث",
      "theme-light":"تغییر به پوستهٔ روشن", "theme-dark":"تغییر به پوستهٔ تیره"
    }
  };

  const root = document.documentElement;
  const langButton = document.getElementById("language-toggle");
  const themeButton = document.getElementById("theme-toggle");
  let language = "en";
  let theme = root.dataset.theme === "light" ? "light" : "dark";

  function localize(languageCode) {
    language = languageCode;
    const dictionary = translations[languageCode];
    root.lang = languageCode;
    root.dir = languageCode === "fa" ? "rtl" : "ltr";
    if (langButton) {
      langButton.textContent = languageCode === "fa" ? "English" : "فارسی";
      langButton.lang = languageCode === "fa" ? "en" : "fa";
      langButton.setAttribute("aria-label", languageCode === "fa" ? "Switch to English" : "نمایش فارسی");
    }
    document.querySelectorAll("[data-t]").forEach((element) => {
      const value = dictionary[element.dataset.t];
      if (typeof value !== "string") return;
      if (element.hasAttribute("data-ta")) {
        element.dataset.ta.split(",").map((item) => item.trim()).filter(Boolean).forEach((attribute) => element.setAttribute(attribute, value));
      } else {
        element.textContent = value;
      }
    });
    if (themeButton) {
      const key = theme === "dark" ? "theme-light" : "theme-dark";
      const themeText = dictionary[key];
      themeButton.setAttribute("aria-label", themeText);
      themeButton.setAttribute("title", themeText);
    }
    updateSearch();
    updatePauseLabel();
    updateProjection();
  }

  function setTheme(nextTheme) {
    theme = nextTheme;
    root.dataset.theme = nextTheme;
    if (themeButton) {
      const dictionary = translations[language];
      const label = dictionary[nextTheme === "dark" ? "theme-light" : "theme-dark"];
      themeButton.setAttribute("aria-label", label);
      themeButton.setAttribute("title", label);
    }
  }

  if (langButton) langButton.addEventListener("click", () => localize(language === "en" ? "fa" : "en"));
  if (themeButton) themeButton.addEventListener("click", () => setTheme(theme === "dark" ? "light" : "dark"));

  const searchInput = document.getElementById("docs-search");
  const docCards = Array.from(document.querySelectorAll(".doc-card"));
  const searchSummary = document.getElementById("search-summary");
  const emptySearch = document.getElementById("docs-empty");

  function updateSearch() {
    if (!searchInput) return;
    const query = searchInput.value.trim().toLocaleLowerCase(language === "fa" ? "fa" : "en");
    let shown = 0;
    for (const card of docCards) {
      const corpus = ((card.dataset.search || "") + " " + card.textContent).toLocaleLowerCase();
      const matches = !query || corpus.includes(query);
      card.hidden = !matches;
      if (matches) shown += 1;
    }
    if (emptySearch) emptySearch.hidden = shown !== 0;
    if (searchSummary) {
      const template = translations[language]["search-count"];
      searchSummary.textContent = query ? template.replace("{shown}", String(shown)).replace("{total}", String(docCards.length)) : translations[language]["search-summary"];
    }
  }

  if (searchInput) {
    searchInput.addEventListener("input", updateSearch);
    document.addEventListener("keydown", (event) => {
      if (event.key !== "/" || event.ctrlKey || event.metaKey || event.altKey) return;
      const active = document.activeElement;
      if (active && /INPUT|TEXTAREA|SELECT/.test(active.tagName)) return;
      event.preventDefault();
      searchInput.focus();
    });
  }

  const copyButton = document.getElementById("copy-command");
  const command = document.getElementById("build-command");
  if (copyButton && command) {
    copyButton.addEventListener("click", async () => {
      const original = translations[language]["copy-command"];
      try {
        if (navigator.clipboard && window.isSecureContext) {
          await navigator.clipboard.writeText(command.textContent);
        } else {
          const temporary = document.createElement("textarea");
          temporary.value = command.textContent;
          temporary.setAttribute("readonly", "");
          temporary.className = "clipboard-fallback";
          document.body.append(temporary);
          temporary.select();
          const ok = document.execCommand("copy");
          temporary.remove();
          if (!ok) throw new Error("Clipboard command was rejected");
        }
        copyButton.textContent = translations[language]["copy-success"];
      } catch (error) {
        copyButton.textContent = translations[language]["copy-failed"];
      }
      window.setTimeout(() => { copyButton.textContent = original; }, 1800);
    });
  }

  const canvas = document.getElementById("engine-canvas");
  const pauseButton = document.getElementById("demo-pause");
  const resetButton = document.getElementById("demo-reset");
  const status = document.getElementById("demo-status");
  const reducedMotion = window.matchMedia("(prefers-reduced-motion: reduce)");
  let ctx = null;
  let dpr = 1;
  let cssWidth = 900;
  let cssHeight = 480;
  let player = { x: 0.36, y: 0.55, vx: 0, vy: 0 };
  let paused = false;
  let intersecting = false;
  let lastFrame = 0;
  let elapsed = 0;
  let frameHandle = 0;

  function resizeCanvas() {
    if (!canvas) return;
    const rect = canvas.getBoundingClientRect();
    if (rect.width < 1 || rect.height < 1) return;
    cssWidth = rect.width;
    cssHeight = rect.height;
    dpr = Math.min(window.devicePixelRatio || 1, 2);
    const newWidth = Math.round(cssWidth * dpr);
    const newHeight = Math.round(cssHeight * dpr);
    if (canvas.width !== newWidth || canvas.height !== newHeight) {
      canvas.width = newWidth;
      canvas.height = newHeight;
    }
    ctx = canvas.getContext("2d", { alpha: false });
    if (ctx) ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    draw(0);
  }

  function draw(timeSeconds) {
    if (!ctx) return;
    ctx.save();
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    ctx.fillStyle = "#0a141e";
    ctx.fillRect(0, 0, cssWidth, cssHeight);
    const spacing = 30;
    ctx.lineWidth = 1;
    ctx.strokeStyle = "rgba(115,160,180,.11)";
    ctx.beginPath();
    for (let x = 0; x < cssWidth; x += spacing) { ctx.moveTo(x, 0); ctx.lineTo(x, cssHeight); }
    for (let y = 0; y < cssHeight; y += spacing) { ctx.moveTo(0, y); ctx.lineTo(cssWidth, y); }
    ctx.stroke();

    const originX = cssWidth * 0.5;
    const originY = cssHeight * 0.5;
    ctx.strokeStyle = "rgba(112,173,191,.3)";
    ctx.setLineDash([5, 6]);
    ctx.beginPath();
    ctx.moveTo(originX, 0); ctx.lineTo(originX, cssHeight);
    ctx.moveTo(0, originY); ctx.lineTo(cssWidth, originY);
    ctx.stroke();
    ctx.setLineDash([]);

    for (let index = 0; index < 10; index += 1) {
      const drift = reducedMotion.matches ? 0 : timeSeconds * (index % 2 ? -3 : 4);
      const px = ((index * 103 + 47 + drift) % (cssWidth + 24) + cssWidth + 24) % (cssWidth + 24);
      const py = 30 + ((index * 67) % Math.max(50, cssHeight - 55));
      ctx.fillStyle = "rgba(124,194,205,.22)";
      ctx.beginPath(); ctx.arc(px, py, index % 3 === 0 ? 2 : 1.2, 0, Math.PI * 2); ctx.fill();
    }

    ctx.fillStyle = "#6c98ad";
    ctx.beginPath(); ctx.arc(originX, originY, 4, 0, Math.PI * 2); ctx.fill();

    const px = player.x * cssWidth;
    const py = player.y * cssHeight;
    ctx.strokeStyle = "rgba(143,247,210,.18)";
    ctx.lineWidth = 1;
    ctx.beginPath(); ctx.arc(px, py, 26 + (reducedMotion.matches ? 0 : Math.sin(timeSeconds * 2) * 3), 0, Math.PI * 2); ctx.stroke();
    ctx.fillStyle = "rgba(83,224,183,.12)";
    ctx.beginPath(); ctx.arc(px, py, 18, 0, Math.PI * 2); ctx.fill();
    ctx.fillStyle = "#8ff7d2";
    ctx.fillRect(px - 8, py - 8, 16, 16);
    ctx.strokeStyle = "#dffff5";
    ctx.strokeRect(px - 8, py - 8, 16, 16);
    ctx.restore();
  }

  function stopLoop() {
    if (frameHandle) window.cancelAnimationFrame(frameHandle);
    frameHandle = 0;
    lastFrame = 0;
  }

  function runFrame(timestamp) {
    frameHandle = 0;
    if (paused || !intersecting || document.hidden || reducedMotion.matches) return;
    const delta = lastFrame ? Math.min((timestamp - lastFrame) / 1000, 0.05) : 0;
    lastFrame = timestamp;
    elapsed += delta;
    player.x += player.vx * delta;
    player.y += player.vy * delta;
    if (player.x < 0.05 || player.x > 0.95) player.vx *= -1;
    if (player.y < 0.08 || player.y > 0.92) player.vy *= -1;
    player.x = Math.max(0.05, Math.min(0.95, player.x));
    player.y = Math.max(0.08, Math.min(0.92, player.y));
    draw(elapsed);
    frameHandle = window.requestAnimationFrame(runFrame);
  }

  function syncLoop() {
    const shouldAnimate = canvas && intersecting && !paused && !document.hidden && !reducedMotion.matches;
    if (shouldAnimate && !frameHandle) frameHandle = window.requestAnimationFrame(runFrame);
    if (!shouldAnimate) stopLoop();
  }

  function updatePauseLabel() {
    if (!pauseButton) return;
    pauseButton.textContent = translations[language][paused ? "resume" : "pause"];
    if (status) status.textContent = translations[language][paused ? "demo-paused" : "demo-ready"] || (paused ? "Motion paused" : "Sandbox ready");
  }

  function move(direction) {
    const step = 0.045;
    if (direction === "left") player.x -= step;
    if (direction === "right") player.x += step;
    if (direction === "up") player.y -= step;
    if (direction === "down") player.y += step;
    player.x = Math.max(0.05, Math.min(0.95, player.x));
    player.y = Math.max(0.08, Math.min(0.92, player.y));
    draw(elapsed);
  }

  if (canvas) {
    const initialContext = canvas.getContext("2d", { alpha: false });
    ctx = initialContext;
    resizeCanvas();
    if (typeof ResizeObserver === "function") new ResizeObserver(resizeCanvas).observe(canvas);
    else window.addEventListener("resize", resizeCanvas, { passive: true });

    canvas.addEventListener("pointerdown", (event) => {
      const rect = canvas.getBoundingClientRect();
      if (!rect.width || !rect.height) return;
      player.x = Math.max(0.05, Math.min(0.95, (event.clientX - rect.left) / rect.width));
      player.y = Math.max(0.08, Math.min(0.92, (event.clientY - rect.top) / rect.height));
      canvas.focus({ preventScroll: true });
      draw(elapsed);
    });
    canvas.addEventListener("keydown", (event) => {
      const keys = { ArrowLeft:"left", a:"left", A:"left", ArrowRight:"right", d:"right", D:"right", ArrowUp:"up", w:"up", W:"up", ArrowDown:"down", s:"down", S:"down" };
      const direction = keys[event.key];
      if (!direction) return;
      event.preventDefault();
      move(direction);
    });
    document.querySelectorAll("[data-move]").forEach((button) => button.addEventListener("click", () => move(button.dataset.move)));
    if (pauseButton) pauseButton.addEventListener("click", () => {
      paused = !paused;
      updatePauseLabel();
      syncLoop();
      draw(elapsed);
    });
    if (resetButton) resetButton.addEventListener("click", () => {
      player = { x:0.36, y:0.55, vx:0, vy:0 };
      elapsed = 0;
      draw(0);
      if (status) status.textContent = language === "fa" ? "موقعیت بازنشانی شد" : "Position reset";
    });
    if (typeof IntersectionObserver === "function") new IntersectionObserver((entries) => {
      intersecting = Boolean(entries[0] && entries[0].isIntersecting);
      syncLoop();
    }, { threshold:0.05 }).observe(canvas);
    else intersecting = true;
    document.addEventListener("visibilitychange", syncLoop);
    window.addEventListener("resize", syncLoop, { passive:true });
    reducedMotion.addEventListener ? reducedMotion.addEventListener("change", syncLoop) : reducedMotion.addListener(syncLoop);
  } else {
    if (status) status.textContent = "Canvas not available";
    if (pauseButton) pauseButton.disabled = true;
    if (resetButton) resetButton.disabled = true;
  }


  // This browser-only lab mirrors the engine's explicit right-handed -Z perspective convention.
  // Math.fround keeps displayed intermediates near the engine's float32 representation.
  const projectionCanvas = document.getElementById("projection-canvas");
  const projectionFov = document.getElementById("projection-fov");
  const projectionFovValue = document.getElementById("projection-fov-value");
  const projectionAspect = document.getElementById("projection-aspect");
  const projectionNear = document.getElementById("projection-near");
  const projectionFar = document.getElementById("projection-far");
  const projectionDepth = document.getElementById("projection-depth");
  const projectionMatrix = document.getElementById("projection-matrix");
  const projectionNearResult = document.getElementById("projection-near-result");
  const projectionFarResult = document.getElementById("projection-far-result");
  const projectionStatus = document.getElementById("projection-status");
  let projectionContext = null;
  let projectionDpr = 1;
  let projectionWidth = 900;
  let projectionHeight = 360;

  function projectionNumber(value) {
    if (!Number.isFinite(value)) return "—";
    const normalized = Object.is(value, -0) ? 0 : value;
    if (normalized !== 0 && (Math.abs(normalized) >= 10000 || Math.abs(normalized) < 0.0001)) {
      return normalized.toExponential(2);
    }
    return normalized.toFixed(4);
  }

  function drawProjectionDiagram(fovDegrees, aspect, near, far) {
    if (!projectionContext) return;
    const ctx = projectionContext;
    const width = projectionWidth;
    const height = projectionHeight;
    ctx.save();
    ctx.setTransform(projectionDpr, 0, 0, projectionDpr, 0, 0);
    ctx.fillStyle = "#0a141e";
    ctx.fillRect(0, 0, width, height);
    ctx.strokeStyle = "rgba(115,160,180,.10)";
    ctx.lineWidth = 1;
    ctx.beginPath();
    for (let x = 0; x <= width; x += 28) { ctx.moveTo(x, 0); ctx.lineTo(x, height); }
    for (let y = 0; y <= height; y += 28) { ctx.moveTo(0, y); ctx.lineTo(width, y); }
    ctx.stroke();

    const safeFov = Number.isFinite(fovDegrees) && fovDegrees > 0 && fovDegrees < 180 ? fovDegrees : 70;
    const safeAspect = Number.isFinite(aspect) && aspect > 0 ? Math.min(aspect, 4) : 16 / 9;
    const cameraX = width * 0.5;
    const cameraY = height * 0.105;
    const tangent = Math.tan(safeFov * Math.PI / 360);
    const fovWidth = Math.max(0.08, Math.min(1.8, tangent * safeAspect));
    const nearHalf = Math.max(16, Math.min(width * 0.19, width * fovWidth * 0.085));
    const depthRatio = Number.isFinite(near) && Number.isFinite(far) && near > 0 && far > near
      ? Math.sqrt(Math.min(far / near, 256)) : 4;
    const farHalf = Math.max(nearHalf + 12, Math.min(width * 0.44, nearHalf * Math.max(1.6, depthRatio)));
    const nearY = height * 0.49;
    const farY = height * 0.84;

    // Schematic rays and planes: the labels intentionally warn that this is not a metric scale.
    ctx.strokeStyle = "rgba(131,202,255,.86)";
    ctx.lineWidth = 1.8;
    ctx.beginPath();
    ctx.moveTo(cameraX, cameraY + 7); ctx.lineTo(cameraX - nearHalf, nearY);
    ctx.moveTo(cameraX, cameraY + 7); ctx.lineTo(cameraX + nearHalf, nearY);
    ctx.moveTo(cameraX, cameraY + 7); ctx.lineTo(cameraX - farHalf, farY);
    ctx.moveTo(cameraX, cameraY + 7); ctx.lineTo(cameraX + farHalf, farY);
    ctx.stroke();
    ctx.strokeStyle = "rgba(143,247,210,.95)";
    ctx.lineWidth = 2;
    ctx.beginPath(); ctx.moveTo(cameraX - nearHalf, nearY); ctx.lineTo(cameraX + nearHalf, nearY); ctx.stroke();
    ctx.strokeStyle = "rgba(243,199,116,.95)";
    ctx.beginPath(); ctx.moveTo(cameraX - farHalf, farY); ctx.lineTo(cameraX + farHalf, farY); ctx.stroke();

    ctx.fillStyle = "#8ff7d2";
    ctx.beginPath(); ctx.arc(cameraX, cameraY, 4.5, 0, Math.PI * 2); ctx.fill();
    ctx.fillStyle = "#eaf3f6";
    ctx.font = "700 10px ui-monospace, monospace";
    ctx.textAlign = "center";
    ctx.direction = language === "fa" ? "rtl" : "ltr";
    const labels = translations[language];
    ctx.fillText(labels["projection-camera"], cameraX, Math.max(12, cameraY - 9));
    ctx.textAlign = "left";
    ctx.fillStyle = "#8ff7d2";
    ctx.fillText(labels["projection-near-name"] + " · " + (Number.isFinite(near) ? String(near) : "—"), Math.min(width - 150, cameraX + nearHalf + 12), nearY + 4);
    ctx.fillStyle = "#f3c774";
    ctx.fillText(labels["projection-far-name"] + " · " + (Number.isFinite(far) ? String(far) : "—"), Math.min(width - 150, cameraX + farHalf + 12), farY + 4);
    ctx.textAlign = "center";
    ctx.fillStyle = "rgba(166,186,197,.8)";
    ctx.font = "9px ui-monospace, monospace";
    ctx.fillText(labels["projection-not-to-scale"], width * 0.5, height - 8);
    ctx.restore();
  }

  function updateProjection() {
    if (!projectionFov || !projectionAspect || !projectionNear || !projectionFar ||
        !projectionDepth || !projectionMatrix || !projectionStatus) return;

    const fovDegrees = Number(projectionFov.value);
    const aspect = Number(projectionAspect.value);
    const nearInput = Number(projectionNear.value);
    const farInput = Number(projectionFar.value);
    if (projectionFovValue) projectionFovValue.textContent = Number.isFinite(fovDegrees) ? fovDegrees + "°" : "—";

    drawProjectionDiagram(fovDegrees, aspect, nearInput, farInput);
    const validPlanes = Number.isFinite(nearInput) && Number.isFinite(farInput) &&
      nearInput > 0 && farInput > nearInput;
    const validBasic = Number.isFinite(fovDegrees) && fovDegrees > 0 && fovDegrees < 180 &&
      Number.isFinite(aspect) && aspect > 0 && validPlanes;
    if (!validBasic) {
      projectionMatrix.textContent = "—";
      projectionNearResult.textContent = translations[language]["projection-invalid-planes"];
      projectionFarResult.textContent = "";
      projectionStatus.textContent = translations[language]["projection-invalid-planes"];
      return;
    }

    const f32 = Math.fround;
    const fovRadians = f32(fovDegrees * Math.PI / 180);
    const tangent = f32(Math.tan(f32(fovRadians * 0.5)));
    const focal = f32(1 / tangent);
    const aspect32 = f32(aspect);
    const near32 = f32(nearInput);
    const far32 = f32(farInput);
    const denominator = f32(near32 - far32);
    const range = projectionDepth.value;
    const supportedRange = range === "zero-to-one" || range === "negative-one-to-one";
    const sum = f32(far32 + near32);
    const product = f32(far32 * near32);
    const m00 = f32(focal / aspect32);
    const m11 = focal;
    const m22 = range === "zero-to-one" ? f32(far32 / denominator) : f32(sum / denominator);
    const m23 = range === "zero-to-one" ? f32(product / denominator) : f32(f32(2 * product) / denominator);
    const matrixValues = [m00, m11, m22, m23, -1, tangent, focal, denominator, product];
    const validMath = supportedRange && Number.isFinite(tangent) && tangent > 0 &&
      Number.isFinite(focal) && focal > 0 && Number.isFinite(denominator) && denominator !== 0 &&
      matrixValues.every(Number.isFinite) && matrixValues.every(value => Number.isFinite(f32(value)));
    if (!validMath) {
      projectionMatrix.textContent = "—";
      projectionNearResult.textContent = translations[language]["projection-invalid-math"];
      projectionFarResult.textContent = "";
      projectionStatus.textContent = translations[language]["projection-invalid-math"];
      return;
    }

    const matrix = [
      [m00, 0, 0, 0],
      [0, m11, 0, 0],
      [0, 0, m22, m23],
      [0, 0, -1, 0]
    ];
    projectionMatrix.textContent = matrix.map(row => "| " + row.map(value => projectionNumber(value).padStart(9, " ")).join("  ") + " |").join("\n");

    const nearClip = f32(f32(m22 * f32(-near32)) + m23);
    const farClip = f32(f32(m22 * f32(-far32)) + m23);
    const nearNdc = f32(nearClip / near32);
    const farNdc = f32(farClip / far32);
    projectionNearResult.textContent = translations[language]["projection-near-result"].replace("{value}", projectionNumber(nearNdc));
    projectionFarResult.textContent = translations[language]["projection-far-result"].replace("{value}", projectionNumber(farNdc));
    projectionStatus.textContent = translations[language]["projection-ready"];
  }

  function resizeProjectionCanvas() {
    if (!projectionCanvas) return;
    const rect = projectionCanvas.getBoundingClientRect();
    if (rect.width < 1 || rect.height < 1) return;
    projectionWidth = rect.width;
    projectionHeight = rect.height;
    projectionDpr = Math.min(window.devicePixelRatio || 1, 2);
    const targetWidth = Math.max(1, Math.round(projectionWidth * projectionDpr));
    const targetHeight = Math.max(1, Math.round(projectionHeight * projectionDpr));
    if (projectionCanvas.width !== targetWidth || projectionCanvas.height !== targetHeight) {
      projectionCanvas.width = targetWidth;
      projectionCanvas.height = targetHeight;
    }
    projectionContext = projectionCanvas.getContext("2d", { alpha: false });
    if (projectionContext) projectionContext.setTransform(projectionDpr, 0, 0, projectionDpr, 0, 0);
    updateProjection();
  }

  [projectionFov, projectionAspect, projectionNear, projectionFar, projectionDepth].filter(Boolean).forEach(control => {
    control.addEventListener("input", updateProjection);
    control.addEventListener("change", updateProjection);
  });
  if (projectionCanvas) {
    if (typeof ResizeObserver === "function") new ResizeObserver(resizeProjectionCanvas).observe(projectionCanvas);
    else window.addEventListener("resize", resizeProjectionCanvas, { passive: true });
    resizeProjectionCanvas();
  }

  const footerYear = document.getElementById("current-year");
  if (footerYear) footerYear.textContent = String(new Date().getFullYear());
  localize("en");
  setTheme(theme);
  if (reducedMotion.matches) draw(0);
})();
