select *
from(select id, shipcountry, 'NorthAmerica' from 'order' where shipcountry = 'USA' or shipcountry = 'Mexico' or shipcountry = 'Canada'
union
select id, shipcountry, 'OtherPlace' from 'order' where shipcountry <> 'USA' and shipcountry <> 'Mexico' and shipcountry <> 'Canada'
)
where id >=15445
order by id limit 20
;



select id, shipcountry, case when shipcountry in ('USA', 'Mexico', 'Canada') then 'NorthAmerica' else 'OtherPlace' end
from 'order'
where id >= 15445
order by id limit 20
;

