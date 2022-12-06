select companyname, (round(cast(late as double)/sumtotal*100,2)) as percentage
from(select companyname, count(*) as late
from 'order', shipper
where shipper.id = 'order'.shipvia and shippeddate > requireddate
group by companyname) natural join
(select companyname, count(*) as sumtotal
from 'order', shipper
where shipper.id = 'order'.shipvia
group by companyname)
order by percentage desc
;

