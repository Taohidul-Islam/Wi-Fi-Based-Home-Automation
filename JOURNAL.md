<h1>Learned How to Design a PCB</h1>
<h2>December 11, 2025</h2>
<h3>Spent time: 2.8Hours</h3>
<p>Today through youtube, I learned how to design a PCB on Kikad and started building my first PCB design. Before I worked with breadboard circuitry but never designed a PCB. The one I am designing right now is not related to any project. It's just an tutorial piece. I am gonna make my main project PCB next day. The whole circuitry is on my head right now.
</p>
<img width="2820" height="1440" alt="image" src="https://github.com/user-attachments/assets/f9528ade-91ad-4950-a86b-b48b1fc89f4f" />
<img width="1921" height="1168" alt="image" src="https://github.com/user-attachments/assets/83e0c371-ca37-4c1e-ac0c-c64646db7ead" />
<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/69f16233-efa4-45a1-8acd-7873be6dcb22" />

 
<h1>Completed my first PCB</h1>
<h2>December 12, 2025</h2>
<h3>Spent time:2.5Hours</h3>
<p>Today I completed my basic learning on building PCB and built an mini inverter PCB watching a tutorial. I leant how to make Gerber file and what to send to PCB manufacturer to manufacture my PCB. I hope from the next day, I can start building my project PCB.
</p>
<img width="3360" height="1080" alt="image" src="https://github.com/user-attachments/assets/33f80ba3-e70b-45db-9a7c-ccbedbea262d" />
<img width="3360" height="1080" alt="image" src="https://github.com/user-attachments/assets/a10ab6a9-b57e-4eff-99f1-2f9e9091197c" />


<h1>Started making Schematics, researched on how to control AC.</h1>
<h2>December 13, 2025</h2>
<h3>Spent time: 4 Hours</h3>
<p> Today I started building my project PCB and done research on how to control AC average voltage with a TRIAC and ESP 32. I want to make a board which can be attached to any switch board and turn it into a smart switchboard. I want to also want to put an AC induction fan regulator. That's why I needed to learn about Triacs. But controlling AC voltage without a transformer is pretty hard ngl. Bought some things to practically learn about TRIACS.
</p>
Today I started building my project PCB and done research on how to control AC average voltage with a TRIAC and ESP 32. Bought some things to practically learn about TRIACS.
<img width="2168" height="2879" alt="image" src="https://github.com/user-attachments/assets/a4b9b796-0382-4075-a8f2-f87552052d20" />
<img width="3360" height="1080" alt="image" src="https://github.com/user-attachments/assets/ddfe9feb-7a91-49bd-8465-c501e084e069" />
<img width="3360" height="1080" alt="image" src="https://github.com/user-attachments/assets/0cdc1797-d946-41f4-9b0b-2a0e7314b452" />


<h1>Finished doing Schematics</h1>
<h2>December 15, 2025</h2>
<h3>Spent time: 8 Hours</h3>
<p>I am journaling after two days as I was busy researching on how to control AC voltage via ESP 32. I couldn't get the TRIACS work so I switched to capacitor based step regulator. And after calculating my preferences and everything it took me a lot of time to do the schematics properly. I used a lot of labels as so many wirings might make it look messy. I switched to the capacitor method cause that is mostly traditional and many people used this kind off setup before me. I failed in controlling TRIACS on my demo model, so I gave up on it and switched to Capacitor based voltage control for AC induction FAN.</p>
<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/529b7d91-9933-4ba3-a43b-73a68decfe84" />


<h1>Modified Schematics for safety</h1>
<h2>December 17, 2025</h2>
<h3>Spent time: 5 Hours</h3>
<p> Modified my final schematics to make it safe and sustainable against AC currents. Did a lot of research on making it safe and placed apropiate safety measures. I mainly added fused to each Channel so if anything inside the circuit failed, the board doesn't become a fire hazard. I also tried to put some place for Varistors for overvoltage protection. I want my board to be like this that if it fails due to any circumstances, the main switch board works perfectly fine.
 </p>
<img width="1921" height="1042" alt="image" src="https://github.com/user-attachments/assets/2b90583e-9498-4c77-ae66-76edf01b3f8f" />
<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/db7efee3-20cf-45ad-9d97-b5669271a35a" />
<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/f672d4d8-fb3f-4cc4-ad28-09ecef5ab12d" />


<h1>Started Designing PCB</h1>
<h2>December 18, 2025</h2>
<h3>Spent time: 2.5 Hours</h3>
<p>I started to design PCB today. First I assigned footprint to all of the components and started making PCB design. Researched how to put it on right place and decided where to put which components and started doing it. I wanted to complete routing today but couldn't due to limited time. I want to make it as compact as possible while maintaining the board design easy to understand and use, so anyone can do the wirings smoothly. So I needed to do put all screw terminal on a single side. But I did a silly mistake. In the 3d model, the screw hole was faced inward but I wanted it to be outward. Because the problem was with only 3D model and the routing can't be neater by putting the terminal to the expected side, I chose to keep the the 3D model as it is. While putting together the components, I just have to flip the terminals and that solves everything. I couldn't edit the 3d model as it was flipping the terminal and ruining my routing blueprint.</p>
<img width="1915" height="1078" alt="image" src="https://github.com/user-attachments/assets/c42e8dc0-39b2-4753-bd76-544412d51d84" />


<h1>Routed the PCB and Completed the hardware part.</h1>
<h2>December 22, 2025</h2>
<h3>Spent Time: 8 Hours</h3>
<p>Today I completed all my work of hardware on Kikad. I completed my PCB designing ensuring no error persisted. I checked it many times and found flaws until it showed no errors. I fixed all my errors and flaws and made it a workable PCB. I suffered a lot while choosing apropiate size for my paths in PCB, so I asked Gemini and gave him a list of what voltage and what current I want to use in what path, he suggested some width but I used a little bit more width than recommended because it gives a bit more headroom. While filling the PCB to GND, I kept all the AC traces away from the fill to prevent and leakage. Adjusting gap from pad to fill took a lot of time. I forgot that I increased the global distance from pad to fill a lot and was trying to adjust the distance in my tab so that wasn't working. After hours of not noticing it, I finally remembered that my global value was high. Then I decreased the global value and after that I could adjust my project value. Sometimes small silly things take more time than important things. Placing Via connection on right place with right radius and padding was also a bit tricky. I avoided via on high voltage AC side. I am very happy today.
</p>
<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/4895bb0c-96e6-4cc8-8dd8-5951951495da" />
<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/921b6704-4bb3-4674-9b8c-a964246a9308" />
<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/966b375d-550e-4cb6-a563-8448d261711c" />
<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/2adaf223-7452-4795-b412-efecd250871c" />



<h1>Completed the coding part.</h1>
<h2>December 23, 2025</h2>
<h3>Spent time: 4 Hours</h3>
<p>
Today I wrote whole logic of my my system and used AI to code more efficiently. I tested all logics by lighting LED and ensuring signal is going properly wherever is need. The logic is like this,
Default, Switch On and Switch On means Low signal doing nothing. This will allow the NC gate of relay to work perfectly without interacting with the microcontroller.
Then when I press the switch On, microcontroller pin gives High signal which is transmitted to ULN transistor array that pulls the negative side of relay coil to ground thus activating the relay and turning the device off.
The fan regulator one is pretty complex. There is normal On off for fan with no Capacitor adjustment. but if you want, after turning off that normal on off button, you can turn on regulated switch buttons, which can channel electricity from 3 types of capacitor and resistor arrangements and controlling the fans speed within 3 Level.
After powering on the microcontroller, the microcontroller works as an Wi-Fi AP(Access Point) and after connecting to it's wifi network and going to certain IP, we can control the devices by toggling graphical toggle switch within phone browser. And there is a option to put home Wi-Fi SSID and password to connect it to home Wi-Fi and make it accessible from anyone in the Wi-Fi network. That is currently in Wi-Fi LAN but it is possible to put it to WAN through port forwarding if you have a real IP, that is a bit complicated and not suitable for everyone. I am planning to develop an application software to control it without typing IPs and using Browsers.
I currently didn't put any IR control code as I never worked with IR receiver. So I left a place on PCB to put IR receiver so after I learn about it, I can add it. This would make the thing more versatile by not depending only on WI-Fi Communications.
</p>
<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/21533a67-e105-4f3f-9421-3deba3530201" />




<h1>Enriched Journal Entries and BOM</h1>
<h2>January 14, 2026</h2>
<h3>Spent 2.5 Hours</h3>
<p>After getting reviewed by blueprint team, I needed to convert BOM from BDT to USD and to add more information on Journal. And for cost efficiency I needed to choose E-Post delivery on JLC PCB, SO I did that. Today I did nothing but edit BOM, rewriting some journal entries and take checkout screenshots.
I needed to convert USD to BDT and BDT simultaneously with cent accuracy, because I had a lot of components which had prices under a dollar. And due to BDT to USD price fluctuation and inflation, my prices decreased a little bit. I also added shipping charge separately site by site.</p>
<img width="1920" height="1078" alt="image" src="https://github.com/user-attachments/assets/9d8ffde3-0355-4d0f-85f4-a138140e38e4" />


<h1>Updated BOM again</h1>
<h2>February 1, 2026</h2>
<h3>Spent 1 Hour</h3>
<p>After getting reviewed again, I had to change the BOM again to remove the tools I mentioned there. So the cost got a bit low. I also got suggestion on how to make the schematics a bit better and was suggested to make a case for my PCB. But as per I has near to no experience in CAD, learning it would take time, and that's the thing I have least right now. As per my SSC examination(Comparable to O level) is coming close, my parents aren't allowing much screen time, so I couldn't make any larger changes. Changing schematics would require me to re-route the whole PCB, which would take hours. But as far I checked with many AI tools, my project seems to be functional. And as per I am new to PCB designing I might have broken some conventions while designing the PCB. Still I am trying if I can make a simple case.
</p>
<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/6751d984-5c3c-4ea5-bd18-bbc842261d9c" />


<h1>Making Case For the PCB</h1>
<h2>February 2, 2026</h2>
<h3>Spent 4 Hours</h3>
<p>Today I somehow managed some time to make an case for my PCB in fusion 360. Learning this software was not that easy. Whatever, the hardest part was to figure out how to put the PCB from kikad into the case in fusion 360. After wandering for 30 minutes I figured it out and made a step file of my pcb from kikad first then imported that step into my fusion 360. now it looks pretty good. I also put screw holes to assemble it.
</p>
<img width="1920" height="1080" alt="Screenshot (120)" src="https://github.com/user-attachments/assets/ecc3546c-f05d-47a4-b21d-7c8583b2b4e4" />
<img width="1920" height="1080" alt="Screenshot (119)" src="https://github.com/user-attachments/assets/4f75601b-8e70-4eef-99be-ec6e8d816fef" />
<img width="1920" height="1080" alt="Screenshot (118)" src="https://github.com/user-attachments/assets/68fa56d3-c7de-4cb1-8cf7-a4037838e2e2" />
<img width="1920" height="1080" alt="Screenshot (117)" src="https://github.com/user-attachments/assets/00469904-8bb7-4f3a-aa25-1afa735cf581" />
<img width="1920" height="1080" alt="Screenshot (116)" src="https://github.com/user-attachments/assets/5fd1c448-d153-4c74-b555-4d96f50bbc98" />

