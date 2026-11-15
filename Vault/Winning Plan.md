# Winning Plan — NASA Space Apps 2026 (written 2026-11-15)

## Facts found (sources: spaceappschallenge.org 2026/2025 pages, science.nasa.gov; the site itself would not load by direct fetch, so check these on the site)
- Event: **14–15 Nov 2026**. Project submission opens Sat 14 Nov 09:00 and **closes Sun 15 Nov 23:59 local time. No edits or exceptions after the deadline** (2025 rule, assume the same). Edit in the Project tab of the team page.
- 14 challenges in 2026; several are about Moon missions (South Pole surface operations, sunlight and comms windows, outpost operations, astronaut health and safety). **Pick the challenge our game answers, and name it in the submission.** Entries under "Create Your Own Challenge" are NOT eligible for global judging.
- Judging criteria (2020/21 wording, 2025 guide still lists five; confirm in the 2026 Judging & Awards Guide): **Impact, Creativity, Validity (scientifically valid), Relevance (answers the challenge), Presentation** (the guide stresses presentation matters as much as the other four).
- Process: local judging picks projects for global judging; then Global Nominees, Finalists, Honorable Mentions (December), Global Winners (January). 2025: 10 global winners from 11,500+ submissions (about 0.1%). A game has won before (2025 Best Mission Concept list included game-style projects; verify on the awards page).
- Past global award names: Best Use of Science, Best Use of Data, Best Use of Technology, Galactic Impact, Best Mission Concept, Most Inspirational, Best Storytelling, Global Connection. Aim at **Best Mission Concept** and **Best Storytelling**.
- 2025 submission rules: the project page does not allow video/document upload; a **Project Demo is mandatory** (a link, e.g. a hosted video). Confirm the 2026 guide.

## Honest odds (my estimates, not data)
| Outcome | Now (untested build) | If the checklist below is done |
|---|---|---|
| Valid, complete submission by the deadline | 70% | 95% |
| Local win / local nominee | 10–15% | 25–35% |
| Global Nominee | 2–4% | 6–10% |
| Global Winner (top 10 of 11,500) | under 1% | 1–3% |

## What decides it (ranked by effect on odds)
1. **Submit before 23:59 local time.** A late or broken submission is a zero. Submit a first version by early evening, then update it before the deadline.
2. **A demo video that cannot fail** (2–3 min, recorded gameplay, voice-over; link hosted on YouTube/Drive unlisted). The judges see this, not the live build. Record S1, then S2, then the S6 summary. Cut stages that are not stable.
3. **Cut scope to what works.** A stable S1 + S2 + summary beats six untested stages. Include S3–S6 only if each runs end to end without a crash; otherwise present them as the roadmap.
4. **Relevance + Validity** in one slide/page: name the NASA challenge; list the real data/models used (NASA 3D Resources, LOLA, Black Marble, Blue Marble, Perseverance, Apollo LM); one line per stage tying it to a real mission step (launch, trans-lunar coast, descent, surface drilling, sample storage, re-entry). Keep real facts only; flag game numbers as simplified.
5. **Impact and Creativity:** the angle is *teamwork under information asymmetry* (four roles, nobody can win alone) used as a training/teaching tool for mission operations. Say who benefits (students, outreach, mission-ops education).
6. **Presentation:** clear title, one-sentence pitch, 6–8 screenshots, video, repo link (GitHub is already public at origin), short README with controls and how to run, credits page (`Vault/Credits.md`).
7. **Open-source/licence hygiene:** list every asset licence (done in `Credits.md`); CC-BY needs the credit line (Carwyn Pelley fairing); no NC assets. Say how AI tools were used if the rules ask (check the 2026 FAQ).

## Concrete checklist for today (in order)
1. Finish S3 (in progress). Run the final game build. Close the editor, rebuild the editor target, reopen.
2. One editor agent: create the missing maps (`L_Orbit` S3, `L_Surface`, `L_Samples`, `L_Return`), place each stage's manager, wire the travel chain, import the shortlisted models.
3. **You play it:** the 6-step S1→S2 test (see top of `CONSTRUCTION_PHASE_ICE_RETURN.md`). Report bugs; I fix. Then S3–S6 with `bFreeRoles` for solo.
4. Fix top bugs only; stop adding features at **~16:00 local**.
5. Record the video; write the project text (template below); submit; re-check the page shows the demo link.
6. Package a Windows build (`Tools/Package/package.ps1 -DryRun`, then real; after the ASCII rename use `-Mode Direct`). Put the zip on Drive and link it. Build time is slow, so start it before the deadline evening.
7. Rename to ASCII only if there is time and nobody is building; it is not needed for the submission.

## Submission text template
- **Title:** Mission Crew: Ice Return
- **Pitch:** A 4-player co-op game where a crew builds a spacecraft, launches it to the Moon, lands near the South Pole, drills for ice, protects the samples and returns them to Earth. Each player has a different job and information, so the mission cannot be completed alone.
- **Challenge answered:** (name the 2026 challenge here)
- **Why it matters:** teaches the real sequence and trade-offs of a lunar sample-return mission and teamwork under limited information.
- **Real data used:** NASA 3D Resources models, LRO/LOLA lunar data, Blue/Black Marble Earth imagery, Perseverance rover model, Apollo LM, real launcher capacities (simplified).
- **Tech:** Unreal Engine 5.8, C++, LAN listen-server multiplayer.
- **Honest limits:** numbers are simplified for play; list what is prototype.
- **Links:** demo video, repo (https://github.com/Abulkhair111/m4rs_nasa), download.

## Biggest risks to remove
- Never played end to end (needs your hand test today).
- Maps for S3–S6 do not exist yet.
- Packaged build never tried; second-PC multiplayer never tried.
- READY labels and reach fix are untested online.
