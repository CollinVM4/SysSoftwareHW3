# AI Usage Disclosure Details
**Student Name:** Collin Van Meter, Jadon Milne
**Student ID:** CVM: 5580217, JM: 5625500
**Assignment:** HW3
---
---
## AI Tool #1
### Tool Name
GitHub Copilot
### Version/Model
Claude Sonnet 3.5
### Date(s) Used
oct 29th - oct 31st 
### Specific Parts of Assignment
Code debugging, specifically for off by one errors when calling advanceToken()
And OPR instruction mismatch (due to glazed over eyes)
Verifying our assignment is to spec per instructions
### Prompts Used
We have lots of debug prints going when we were manually debugging so we provided these in our prompts. 
We had gathered it was an off by one error and other logic (the opr calls) just seemed off. So we included our programs output vs expected output to help iron out the discrepancies. 
### AI Output/Results
AI pointed out our mismatch in where we called OPR and what the instruction really should have been. 
It also helped us resolve the flow of calling advanceToken(), tracing through all the function calls line by line was rough, however the AI could quicky follow through the code and 
help us resolve this issue.
### How Output was Verified/Edited
My teammate and I verified and reviewed the code we received back and it instantly clicked. We were pretty much getting "glazed over" eyes and needed a pair of fresh eyes to take a look.
After manually verifying ourselves, we ran several unit tests to check the output of our code.