with employee_plus(employeeid, firstname, lastname, birthdate) as
(select id, firstname, lastname, birthdate
from employee),
employeeterritory_plus(employeeid, territoryid) as
(select employeeid, territoryid
from employeeterritory),
territory_plus(territoryid, regionid) as
(select id, regionid
from territory),
region_plus(regionid, regiondescription) as
(select id, regiondescription
from region)
select regiondescription, firstname, lastname, birthdate
from employee_plus natural join employeeterritory_plus natural join territory_plus natural join region_plus
group by regionid
having max(birthdate)
order by regionid
;
