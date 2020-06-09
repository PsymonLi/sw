Definitions:
specific - specific rules usually prefixes to match all subnets
            the prefixes are generated from subnet and/or local/remote mappings
            src_pfx, dst_pfx, proto, src_port, dst_port
random -  rules containing randomly generated prefixes, proto
            random_pfx, random_pfx, random_proto, random_src_port, random_dst_port
wildcard - wildcard rules that matches any packet
            0.0.0.0/0  0.0.0.0/0  PROTO_ANY 0-65535 0-65535

{Set1, Set2} - A set of rules with subset Set1 having higher priority (lower numeric value)
               than subset Set2
Set1/Set2 - A set of one or more rules

A. Policy combinations testcases:
Policy 1                       Policy 2                      Final result       Status
1. allow all                     allow all                         allow
2. allow all                     specific allow                    allow
3. allow all                     specific deny                     deny
4. allow all                     deny all                          deny
5. deny all                      allow all                         deny
6. deny all                      specific allow                    deny
7. deny all                      deny all                          deny
8. deny all                      specific deny                     deny
9. specific allow                allow all                         allow
10. specific allow               specific allow                    allow
11. specific allow               deny all                          deny
12. specific allow               specific deny                     deny
13. specific deny                allow all                         deny
14. specific deny                deny all                          deny
15. specific deny                specific allow                    deny
16. specific deny                specific deny                     deny

These are the different ways in which policy can achieve the desired behaviour.

Each of the combinations under Desired behaviour will result in that behaviour.
Desired-        Rules                           Rule action     Default fwaction
behaviour

allow all
                empty                               --                  allow
                wildcard rules                      allow               deny
                wildcard rules                      allow               allow
                specific rules                      allow               allow
                {wildcard, specific}                {allow, allow}      deny
                {wildcard, specific}                {allow, deny}       deny
                {specific, wildcard}                {allow, allow}      deny

deny all
                No policy attached in the subnet
                empty policy                        --                  deny
                wildcard rules                      deny                allow
                wildcard rules                      deny                deny
                specific                            deny                deny
                {wildcard, specific}                {deny, allow}       deny
                {wildcard, specific}                {deny, deny}        deny
                {specific, wildcard}                {deny, deny}        deny

specific allow
                specific rules                      allow               deny
                {specific, random}                  {allow, deny}       deny
                {specific, random}                  {allow, deny}       allow
                {specific, wildcard}                {allow, deny}       deny
                {specific, wildcard}                {allow, deny}       allow

specific deny
                specific rules                      deny                allow
                {specific, random}                  {deny, allow}       deny
                {specific, random}                  {deny, deny}        allow
                {specific, wildcard}                {deny, allow}       deny
                {specific, wildcard}                {deny, allow}       allow

B. policy change while the session is in progress should update the session action
- allow -> deny
- deny -> allow
- allow -> allow
- deny -> deny

C. Combinations of policy and security-profile
    policy      security profile
    allow       allow
    allow       deny
    deny        allow
    deny        deny
